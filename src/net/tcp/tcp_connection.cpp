#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include "tcp_connection.h"
#include "tcp_server.h"
#include "coroutine_hook.h"
#include "coroutine_pool.h"
#include "timer.h"

namespace corpc {

TcpConnection::TcpConnection(corpc::TcpServer *tcpServer, corpc::IOThread *ioThread, int fd, int buffSize, NetAddress::ptr peerAddr)
    : ioThread_(ioThread), fd_(fd), state_(Connected), connectionType_(ServerConnection), peerAddr_(peerAddr)
{
    loop_ = ioThread_->getEventLoop();

    tcpServer_ = tcpServer;

    m_codec = tcpServer_->getCodec();
    channel_ = ChannelContainer::getChannelContainer()->getChannel(fd);
    channel_->setEventLoop(loop_);
    initBuffer(buffSize);
    loopCor_ = getCoroutinePool()->getCoroutineInstanse(); // 子协程
    state_ = Connected;
    LOG_DEBUG << "succ create tcp connection[" << state_ << "], fd=" << fd;
}

TcpConnection::TcpConnection(corpc::TcpClient *tcpCli, corpc::EventLoop *loop, int fd, int buffSize, NetAddress::ptr peerAddr)
    : fd_(fd), state_(NotConnected), connectionType_(ClientConnection), peerAddr_(peerAddr)
{
    loop_ = loop;

    tcpClient_ = tcpCli;

    m_codec = tcpClient_->getCodeC();

    channel_ = ChannelContainer::getChannelContainer()->getChannel(fd);
    channel_->setEventLoop(loop_);
    initBuffer(buffSize);

    LOG_DEBUG << "succ create tcp connection[NotConnected]";
}

void TcpConnection::initServer()
{
    loopCor_->setCallBack(std::bind(&TcpConnection::mainServerLoopCorFunc, this));
}

void TcpConnection::setUpServer()
{
    loop_->addCoroutine(loopCor_);
}

void TcpConnection::registerToTimeWheel()
{
    auto cb = [](TcpConnection::ptr conn) {
        conn->shutdownConnection();
    };
    TcpTimeWheel::TcpConnectionSlot::ptr temp = std::make_shared<AbstractSlot<TcpConnection>>(shared_from_this(), cb);
    weakSlot_ = temp;
    tcpServer_->freshTcpConnection(temp);
}

void TcpConnection::setUpClient()
{
    setState(Connected);
}

TcpConnection::~TcpConnection()
{
    if (connectionType_ == ServerConnection) {
        getCoroutinePool()->returnCoroutine(loopCor_);
    }

    LOG_DEBUG << "~TcpConnection, fd=" << fd_;
}

void TcpConnection::initBuffer(int size)
{
    // 初始化缓冲区大小
    writeBuffer_ = std::make_shared<TcpBuffer>(size);
    readBuffer_ = std::make_shared<TcpBuffer>(size);
}

void TcpConnection::mainServerLoopCorFunc()
{
    while (!stop_) {
        input(); // 读数据
        execute(); // 处理数据
        output(); // 写数据
    }
    LOG_INFO << "this connection has already end loop";
}

void TcpConnection::input()
{
    if (isOverTime_) {
        LOG_INFO << "over timer, skip input progress";
        return;
    }
    TcpConnectionState state = getState();
    if (state == Closed || state == NotConnected) {
        return;
    }
    bool readAll = false; // 本轮的数据是否读完
    bool closeFlag = false;
    int count = 0;
    while (!readAll) {
        if (readBuffer_->writeAble() == 0) {
            readBuffer_->resizeBuffer(2 * readBuffer_->getSize());
        }

        int readCount = readBuffer_->writeAble();
        int writeIndex = readBuffer_->writeIndex();

        LOG_DEBUG << "readBuffer_ size=" << readBuffer_->getBufferVector().size() << " rd=" << readBuffer_->readIndex() << " wd=" << readBuffer_->writeIndex();
        int ret = read_hook(fd_, &(readBuffer_->buffer_[writeIndex]), readCount);
        if (ret > 0) {
            readBuffer_->recycleWrite(ret);
        }
        LOG_DEBUG << "readBuffer_ size=" << readBuffer_->getBufferVector().size() << " rd=" << readBuffer_->readIndex() << " wd=" << readBuffer_->writeIndex();

        LOG_DEBUG << "read data back, fd=" << fd_;
        if (isOverTime_) {
            LOG_INFO << "over timer, now break read function";
            break;
        }
        if (ret <= 0) {
            LOG_DEBUG << "ret <= 0";
            LOG_ERROR << "read empty while occur read event, because of peer close, fd= " << fd_ << ", sys error=" << strerror(errno) << ", now to clear tcp connection";
            // this cor can destroy
            closeFlag = true;
            break;
        }
        else {
            count += ret;
            if (ret == readCount) {
                LOG_DEBUG << "readCount == ret";
                // is is possible read more data, should continue read
                continue;
            }
            else if (ret < readCount) {
                LOG_DEBUG << "readCount > ret";
                // read all data in socket buffer, skip out loop
                readAll = true; // 如果本次读出的数据比要读的数据的字节数少，说明本轮数据读完了
                break;
            }
        }
    }
    if (closeFlag) {
        clearClient();
        LOG_DEBUG << "peer close, now yield current coroutine, wait main thread clear this TcpConnection";
        Coroutine::getCurrentCoroutine()->setCanResume(false); // 注意设置这个子协程不能再切换回去，否则会出现错误
        Coroutine::yield();
    }

    if (isOverTime_) {
        return;
    }

    if (!readAll) {
        LOG_ERROR << "not read all data in socket buffer";
    }
    LOG_INFO << "recv [" << count << "] bytes data from [" << peerAddr_->toString() << "], fd [" << fd_ << "]";
    // 连接有新数据来了，需要更新该连接在时间轮中的位置
    if (connectionType_ == ServerConnection) {
        TcpTimeWheel::TcpConnectionSlot::ptr temp = weakSlot_.lock();
        if (temp) {
            tcpServer_->freshTcpConnection(temp);
        }
    }
}

void TcpConnection::execute()
{
    // it only server do this
    while (readBuffer_->readAble() > 0) {
        std::shared_ptr<AbstractData> data;
        if (m_codec->getProtocalType() == TinyPb_Protocal) {
            data = std::make_shared<TinyPbStruct>();
        }
        else {
            data = std::make_shared<HttpRequest>();
        }

        m_codec->decode(readBuffer_.get(), data.get());
        // LOG_DEBUG << "parse service_name=" << pb_struct.service_full_name;
        if (!data->decode_succ) {
            LOG_ERROR << "it parse request error of fd " << fd_;
            break;
        }
        // LOG_DEBUG << "it parse request success";
        if (connectionType_ == ServerConnection) {
            // LOG_DEBUG << "to dispatch this package";
            tcpServer_->getDispatcher()->dispatch(data.get(), this);
            // LOG_DEBUG << "contine parse next package";
        }
        else if (connectionType_ == ClientConnection) {
            // TODO:
            std::shared_ptr<TinyPbStruct> tmp = std::dynamic_pointer_cast<TinyPbStruct>(data);
            if (tmp) {
                m_reply_datas.insert(std::make_pair(tmp->msg_req, tmp));
            }
        }
    }
}

void TcpConnection::output()
{
    if (isOverTime_) {
        LOG_INFO << "over timer, skip output progress";
        return;
    }
    while (true) {
        TcpConnectionState state = getState();
        if (state != Connected) {
            break;
        }

        if (writeBuffer_->readAble() == 0) {
            LOG_DEBUG << "app buffer of fd[" << fd_ << "] no data to write, to yiled this coroutine";
            break;
        }

        int totalSize = writeBuffer_->readAble();
        int readIndex = writeBuffer_->readIndex();
        int ret = write_hook(fd_, &(writeBuffer_->buffer_[readIndex]), totalSize);
        // LOG_INFO << "write end";
        if (ret <= 0) {
            LOG_ERROR << "write empty, error=" << strerror(errno);
        }

        LOG_DEBUG << "succ write " << ret << " bytes";
        writeBuffer_->recycleRead(ret);
        LOG_DEBUG << "recycle write index =" << writeBuffer_->writeIndex() << ", readIndex =" << writeBuffer_->readIndex() << "readable = " << writeBuffer_->readAble();
        LOG_INFO << "send[" << ret << "] bytes data to [" << peerAddr_->toString() << "], fd [" << fd_ << "]";
        if (writeBuffer_->readAble() <= 0) { // 已发送完所有数据
            LOG_INFO << "send all data, now unregister write event and break";
            break;
        }

        if (isOverTime_) {
            LOG_INFO << "over timer, now break write function";
            break;
        }
    }
}

void TcpConnection::clearClient()
{
    if (getState() == Closed) {
        LOG_DEBUG << "this client has closed";
        return;
    }
    // first unregister epoll event
    channel_->unregisterFromEventLoop();

    // stop read and write cor
    stop_ = true;

    close(channel_->getFd());
    setState(Closed);
}

void TcpConnection::shutdownConnection()
{
    TcpConnectionState state = getState();
    if (state == Closed || state == NotConnected) {
        LOG_DEBUG << "this client has closed";
        return;
    }
    setState(HalfClosing);
    LOG_INFO << "shutdown conn[" << peerAddr_->toString() << "], fd=" << fd_;
    // call sys shutdown to send FIN
    // wait client done something, client will send FIN
    // and fd occur read event but byte count is 0
    // then will call clearClient to set CLOSED
    // IOThread::MainLoopTimerFunc will delete CLOSED connection
    shutdown(channel_->getFd(), SHUT_RDWR);
}

TcpBuffer *TcpConnection::getInBuffer()
{
    return readBuffer_.get();
}

TcpBuffer *TcpConnection::getOutBuffer()
{
    return writeBuffer_.get();
}

bool TcpConnection::getResPackageData(const std::string &msg_req, TinyPbStruct::pb_ptr &pb_struct)
{
    auto it = m_reply_datas.find(msg_req);
    if (it != m_reply_datas.end())
    {
        LOG_DEBUG << "return a resdata";
        pb_struct = it->second;
        m_reply_datas.erase(it);
        return true;
    }
    LOG_DEBUG << msg_req << "|reply data not exist";
    return false;
}

AbstractCodeC::ptr TcpConnection::getCodec() const
{
    return m_codec;
}

TcpConnectionState TcpConnection::getState()
{
    TcpConnectionState state;
    RWMutex::ReadLock lock(mutex_);
    state = state_;
    lock.unlock();

    return state;
}

void TcpConnection::setState(const TcpConnectionState &state)
{
    RWMutex::WriteLock lock(mutex_);
    state_ = state;
    lock.unlock();
}

void TcpConnection::setOverTimeFlag(bool value)
{
    isOverTime_ = value;
}

bool TcpConnection::getOverTimerFlag()
{
    return isOverTime_;
}

Coroutine::ptr TcpConnection::getCoroutine()
{
    return loopCor_;
}

}
