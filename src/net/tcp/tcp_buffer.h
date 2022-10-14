#ifndef TCP_BUFFER_H
#define TCP_BUFFER_H

#include <vector>
#include <memory>

namespace corpc {

class TcpBuffer {
public:
    typedef std::shared_ptr<TcpBuffer> ptr;

    explicit TcpBuffer(int size);
    ~TcpBuffer();
    int readAble();
    int writeAble();
    int readIndex() const;
    int writeIndex() const;

    void writeToBuffer(const char *buf, int size);
    void readFromBuffer(std::vector<char> &re, int size);
    void resizeBuffer(int size);
    void clearBuffer();
    int getSize();

    std::vector<char> getBufferVector();
    std::string getBufferString();

    void recycleRead(int index);
    void recycleWrite(int index);

    void adjustBuffer();

private:
    int readIndex_{0};
    int writeIndex_{0};
    int size_{0};

public:
    std::vector<char> buffer_;
};

}

#endif
