#include <unistd.h>
#include <cstring>
#include "corpc/net/tcp/tcp_buffer.h"
#include "corpc/common/log.h"

namespace corpc {

TcpBuffer::TcpBuffer(int size)
{
    buffer_.resize(size);
}

TcpBuffer::~TcpBuffer()
{
}

int TcpBuffer::readAble()
{
    return writeIndex_ - readIndex_;
}

int TcpBuffer::writeAble()
{
    return buffer_.size() - writeIndex_;
}

int TcpBuffer::readIndex() const
{
    return readIndex_;
}

int TcpBuffer::writeIndex() const
{
    return writeIndex_;
}

// 重新设置缓冲区大小；整理缓冲区内容，将可读区域的第一个字节平移到缓冲区0的位置
void TcpBuffer::resizeBuffer(int size)
{
    std::vector<char> temp(size);
    int c = std::min(size, readAble());
    memcpy(&temp[0], &buffer_[readIndex_], c);

    buffer_.swap(temp);
    readIndex_ = 0;
    writeIndex_ = readIndex_ + c;
}

void TcpBuffer::writeToBuffer(const char *buf, int size)
{
    if (size > writeAble()) {
        int newSize = (int)(1.5 * (writeIndex_ + size));
        resizeBuffer(newSize);
    }
    memcpy(&buffer_[writeIndex_], buf, size);
    writeIndex_ += size;
}

void TcpBuffer::readFromBuffer(std::vector<char> &re, int size)
{
    if (readAble() == 0) {
        LOG_DEBUG << "read buffer empty!";
        return;
    }
    int readSize = readAble() > size ? size : readAble();
    std::vector<char> temp(readSize);

    memcpy(&temp[0], &buffer_[readIndex_], readSize);
    re.swap(temp);
    readIndex_ += readSize;
    adjustBuffer();
}

// 整理缓冲区内容，将可读区域的第一个字节平移到缓冲区0的位置
void TcpBuffer::adjustBuffer()
{
    if (readIndex_ > static_cast<int>(buffer_.size() / 3)) {
        std::vector<char> newBuffer(buffer_.size());

        int count = readAble();
        memcpy(&newBuffer[0], &buffer_[readIndex_], count);

        buffer_.swap(newBuffer);
        writeIndex_ = count;
        readIndex_ = 0;
        newBuffer.clear();
    }
}

int TcpBuffer::getSize()
{
    return buffer_.size();
}

void TcpBuffer::clearBuffer()
{
    buffer_.clear();
    readIndex_ = 0;
    writeIndex_ = 0;
}

void TcpBuffer::recycleRead(int index)
{
    int j = readIndex_ + index;
    if (j > (int)buffer_.size()) {
        LOG_ERROR << "recycleRead error";
        return;
    }
    readIndex_ = j;
    adjustBuffer();
}

void TcpBuffer::recycleWrite(int index)
{
    int j = writeIndex_ + index;
    if (j > (int)buffer_.size()) {
        LOG_ERROR << "recycleWrite error";
        return;
    }
    writeIndex_ = j;
    adjustBuffer();
}

std::string TcpBuffer::getBufferString()
{
    std::string re(readAble(), '0');
    memcpy(&re[0], &buffer_[readIndex_], readAble());
    return re;
}

std::vector<char> TcpBuffer::getBufferVector()
{
    return buffer_;
}

}
