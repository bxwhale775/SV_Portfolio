#pragma once
#include <Windows.h>
#include "NetHeader.h"

constexpr int SERIALBUF_HEADERSIZE =	sizeof(NetHeader);
constexpr int SERIALBUF_ENCRYPTFROM =	sizeof(NetHeader::code) +
										sizeof(NetHeader::len) +
										sizeof(NetHeader::randkey);

class SerialBuffer {
protected:
	char* _pbuf = nullptr;
	int _size = 0;
	int _head = 0;
	int _tail = 0;
	long _useCount = 1;
	int _headergap = SERIALBUF_HEADERSIZE;
	bool _isHeaderIncluded = false;
	char _isEncrypted = 0;
public:
	SerialBuffer() {
		_pbuf = nullptr;
		_headergap = SERIALBUF_HEADERSIZE;
		this->Clear();
		return;
	}
	SerialBuffer(int size, int headergap = 0 ) : 
		_size(size),
		_headergap(headergap)
	{
		_pbuf = new char[(size_t)_size + (size_t)_headergap];
		this->Clear();
		return;
	}
	virtual ~SerialBuffer() {
		if (_pbuf != nullptr) { delete[] _pbuf; }
		return;
	}

	bool Init(int size) {
		if (_pbuf != nullptr) { return false; }
		_size = size;
		_pbuf = new char[(size_t)_size + (size_t)_headergap];
		this->Clear();
		return true;
	}

	int GetBufferSize() {
		return _size;
	}
	int GetDataSize(bool excludeheader = false) {
		int ret = _head - _tail;
		if (_isHeaderIncluded && !excludeheader) { ret += _headergap; }
		return ret;
	}
	int getUseCount() {
		return _useCount;
	}
	char* GetBufferPtr() {
		return _pbuf;
	}
	char* GetBufferHeadPtr() {
		return &_pbuf[_head];
	}
	char* GetBufferTailPtr() {
		return &_pbuf[_tail];
	}

	int MoveWritePos(int mov) {		//head movement
		if (mov <= 0) { return 0; }
		if (_size < _head + mov) {
			int ret = _size - _head;
			_head = _size;
			return ret;
		}
		else {
			_head += mov;
			return mov;
		}
	}
	int MoveReadPos(int mov) {		//tail movement
		if (mov <= 0) { return 0; }
		if (_head < _tail + mov) {
			int ret = _head - _tail;
			_tail = _head;
			return ret;
		}
		else {
			_tail += mov;
			return mov;
		}
	}

	long IncUseCount() {
		return InterlockedIncrement(&_useCount);
	}
	long DecUseCount() {
		return InterlockedDecrement(&_useCount);
	}
	void Clear() {
		_head = _headergap;
		_tail = _headergap;
		_useCount = 1;
		_isHeaderIncluded = false;
		_isEncrypted = 0;
		return;
	}

	int GetData(void* pDest, int size);
	int PutData(const void* pSrc, int size);
	void PutHeader(const void* pSrc) {
		if (_isHeaderIncluded) { return; }
		memcpy(_pbuf, pSrc, _headergap);
		_isHeaderIncluded = true;
		return;
	}

	bool Encrypt(BYTE EncryptKey = 0) {
		if (!_isHeaderIncluded) { return false; }
		if (_isEncrypted == 1) { return false; }
		_isEncrypted = 1;

		//calc checksum
		int size = GetDataSize() - _headergap;
		int encsize = GetDataSize() - SERIALBUF_ENCRYPTFROM;
		BYTE chksum = 0;
		char* ptr = &_pbuf[_tail];
		for (int i = 0; i < size; i++) {
			chksum += ptr[i];
		}

		//set checksum and get randkey
		NetHeader* tmp = (NetHeader*)_pbuf;
		tmp->chksum = chksum;
		BYTE randkey = tmp->randkey;

		//do Encrypt
		unsigned char temp_p = 0;
		unsigned char temp_e = 0;
		for (int i = 0; i < encsize; i++) {
			temp_p = (temp_p + randkey + i + 1) ^ _pbuf[SERIALBUF_ENCRYPTFROM + i];
			temp_e = (temp_e + EncryptKey + i + 1) ^ temp_p;
			_pbuf[SERIALBUF_ENCRYPTFROM + i] = temp_e;
		}

		return true;
	}
	bool Decrypt(BYTE EncryptKey = 0) {
		//get header, randkey, size
		NetHeader* header = (NetHeader*)_pbuf;
		BYTE randkey = header->randkey;
		int encsize = GetDataSize() - SERIALBUF_ENCRYPTFROM;


		//do decrypt
		unsigned char dtemp_preve = 0;
		unsigned char dtemp_e = 0;
		unsigned char dtemp_prevp = 0;
		unsigned char dtemp_p = 0;
		for (int i = 0; i < encsize; i++) {
			dtemp_e = _pbuf[SERIALBUF_ENCRYPTFROM + i];
			dtemp_p = (dtemp_preve + EncryptKey + i + 1) ^ dtemp_e;
			_pbuf[SERIALBUF_ENCRYPTFROM + i] = (dtemp_prevp + randkey + i + 1) ^ dtemp_p;
			dtemp_prevp = dtemp_p;
			dtemp_preve = dtemp_e;
		}

		//get checksum from header
		BYTE hdchksum = header->chksum;

		//calc checksum from payload
		BYTE plchksum = 0;
		char* ptr = &_pbuf[sizeof(NetHeader)];
		for (int i = 0; i < header->len; i++) {
			plchksum += ptr[i];
		}

		//valitate checksum
		if (hdchksum != plchksum) {
			return false;
		}
		return true;
	}
	
	//write <<
	SerialBuffer& operator<<(char val);
	SerialBuffer& operator<<(unsigned char val);

	SerialBuffer& operator<<(short val);
	SerialBuffer& operator<<(unsigned short val);

	SerialBuffer& operator<<(int val);
	SerialBuffer& operator<<(unsigned int val);
	SerialBuffer& operator<<(long val);
	SerialBuffer& operator<<(unsigned long val);
	SerialBuffer& operator<<(float val);

	SerialBuffer& operator<<(long long val);
	SerialBuffer& operator<<(unsigned long long val);
	SerialBuffer& operator<<(double val);

	//read >>
	SerialBuffer& operator>>(char& val);
	SerialBuffer& operator>>(unsigned char& val);
	
	SerialBuffer& operator>>(short& val);
	SerialBuffer& operator>>(unsigned short& val);
	
	SerialBuffer& operator>>(int& val);
	SerialBuffer& operator>>(unsigned int& val);
	SerialBuffer& operator>>(long& val);
	SerialBuffer& operator>>(unsigned long& val);
	SerialBuffer& operator>>(float& val);
	
	SerialBuffer& operator>>(long long& val);
	SerialBuffer& operator>>(unsigned long long& val);
	SerialBuffer& operator>>(double& val);
};