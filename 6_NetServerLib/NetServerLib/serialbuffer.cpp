#include "NetSV-include/NetSV/serialbuffer.h"

int SerialBuffer::GetData(void* pDest, int size) {
    int datasize = GetDataSize(true);
    if (size <= 0 || datasize <= 0) { return 0; }
    int cnt = datasize < size ? datasize : size;
    memcpy(pDest, &_pbuf[_tail], cnt);
    MoveReadPos(cnt);
    return cnt;
}

int SerialBuffer::PutData(const void* pSrc, int size) {
    int remainsize = _size - _head;
    if (size <= 0 || remainsize == 0) { return 0; }
    int cnt = remainsize < size ? remainsize : size;
    memcpy(&_pbuf[_head], pSrc, cnt);
    MoveWritePos(cnt);
    return cnt;
}


SerialBuffer& SerialBuffer::operator<<(char val) {
    PutData(&val, sizeof(val));
    return *this;
}

SerialBuffer& SerialBuffer::operator<<(unsigned char val) {
    PutData((char*)&val, sizeof(val));
    return *this;
}

SerialBuffer& SerialBuffer::operator<<(short val) {
    PutData((char*)&val, sizeof(val));
    return *this;
}

SerialBuffer& SerialBuffer::operator<<(unsigned short val) {
    PutData((char*)&val, sizeof(val));
    return *this;
}

SerialBuffer& SerialBuffer::operator<<(int val) {
    PutData((char*)&val, sizeof(val));
    return *this;
}

SerialBuffer& SerialBuffer::operator<<(unsigned int val) {
    PutData((char*)&val, sizeof(val));
    return *this;
}

SerialBuffer& SerialBuffer::operator<<(long val) {
    PutData((char*)&val, sizeof(val));
    return *this;
}

SerialBuffer& SerialBuffer::operator<<(unsigned long val) {
    PutData((char*)&val, sizeof(val));
    return *this;
}

SerialBuffer& SerialBuffer::operator<<(float val) {
    PutData((char*)&val, sizeof(val));
    return *this;
}

SerialBuffer& SerialBuffer::operator<<(long long val) {
    PutData((char*)&val, sizeof(val));
    return *this;
}

SerialBuffer& SerialBuffer::operator<<(unsigned long long val) {
    PutData((char*)&val, sizeof(val));
    return *this;
}

SerialBuffer& SerialBuffer::operator<<(double val) {
    PutData((char*)&val, sizeof(val));
    return *this;
}

SerialBuffer& SerialBuffer::operator>>(char& val) {
    GetData(&val, sizeof(val));
    return *this;
}

SerialBuffer& SerialBuffer::operator>>(unsigned char& val) {
    GetData((char*)&val, sizeof(val));
    return *this;
}

SerialBuffer& SerialBuffer::operator>>(short& val) {
    GetData((char*)&val, sizeof(val));
    return *this;
}

SerialBuffer& SerialBuffer::operator>>(unsigned short& val) {
    GetData((char*)&val, sizeof(val));
    return *this;
}

SerialBuffer& SerialBuffer::operator>>(int& val) {
    GetData((char*)&val, sizeof(val));
    return *this;
}

SerialBuffer& SerialBuffer::operator>>(unsigned int& val) {
    GetData((char*)&val, sizeof(val));
    return *this;
}

SerialBuffer& SerialBuffer::operator>>(long& val) {
    GetData((char*)&val, sizeof(val));
    return *this;
}

SerialBuffer& SerialBuffer::operator>>(unsigned long& val) {
    GetData((char*)&val, sizeof(val));
    return *this;
}

SerialBuffer& SerialBuffer::operator>>(float& val) {
    GetData((char*)&val, sizeof(val));
    return *this;
}

SerialBuffer& SerialBuffer::operator>>(long long& val) {
    GetData((char*)&val, sizeof(val));
    return *this;
}

SerialBuffer& SerialBuffer::operator>>(unsigned long long& val) {
    GetData((char*)&val, sizeof(val));
    return *this;
}

SerialBuffer& SerialBuffer::operator>>(double& val) {
    GetData((char*)&val, sizeof(val));
    return *this;
}


