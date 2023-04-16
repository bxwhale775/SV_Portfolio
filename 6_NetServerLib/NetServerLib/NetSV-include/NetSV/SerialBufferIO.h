#pragma once
#include "serialbuffer.h"

class SerialBufferIO {
public:
	SerialBuffer* _pbuf;
	SerialBufferIO(SerialBuffer* pbuf) : _pbuf(pbuf) { return; }

	//// Add Custom I/O operation function for RPC here

	// << PUT
	

	// >> GET


	////////////////////////////////////

	// << PUT

	SerialBufferIO& operator<<(char val) {
		_pbuf->PutData(&val, sizeof(val));
		return *this;
	}

	SerialBufferIO& operator<<(unsigned char val) {
		_pbuf->PutData(&val, sizeof(val));
		return *this;
	}

	SerialBufferIO& operator<<(short val) {
		_pbuf->PutData(&val, sizeof(val));
		return *this;
	}

	SerialBufferIO& operator<<(unsigned short val) {
		_pbuf->PutData(&val, sizeof(val));
		return *this;
	}

	SerialBufferIO& operator<<(wchar_t val) {
		_pbuf->PutData(&val, sizeof(val));
		return *this;
	}

	SerialBufferIO& operator<<(bool val) {
		_pbuf->PutData(&val, sizeof(val));
		return *this;
	}

	SerialBufferIO& operator<<(int val) {
		_pbuf->PutData(&val, sizeof(val));
		return *this;
	}

	SerialBufferIO& operator<<(unsigned int val) {
		_pbuf->PutData(&val, sizeof(val));
		return *this;
	}

	SerialBufferIO& operator<<(long val) {
		_pbuf->PutData(&val, sizeof(val));
		return *this;
	}

	SerialBufferIO& operator<<(unsigned long val) {
		_pbuf->PutData(&val, sizeof(val));
		return *this;
	}

	SerialBufferIO& operator<<(float val) {
		_pbuf->PutData(&val, sizeof(val));
		return *this;
	}

	SerialBufferIO& operator<<(long long val) {
		_pbuf->PutData(&val, sizeof(val));
		return *this;
	}

	SerialBufferIO& operator<<(unsigned long long val) {
		_pbuf->PutData(&val, sizeof(val));
		return *this;
	}

	SerialBufferIO& operator<<(double val) {
		_pbuf->PutData(&val, sizeof(val));
		return *this;
	}

	// >> GET

	SerialBufferIO& operator>>(char& val) {
		_pbuf->GetData(&val, sizeof(val));
		return *this;
	}

	SerialBufferIO& operator>>(unsigned char& val) {
		_pbuf->GetData(&val, sizeof(val));
		return *this;
	}

	SerialBufferIO& operator>>(short& val) {
		_pbuf->GetData(&val, sizeof(val));
		return *this;
	}

	SerialBufferIO& operator>>(unsigned short& val) {
		_pbuf->GetData(&val, sizeof(val));
		return *this;
	}

	SerialBufferIO& operator>>(wchar_t& val) {
		_pbuf->GetData(&val, sizeof(val));
		return *this;
	}

	SerialBufferIO& operator>>(bool& val) {
		_pbuf->GetData(&val, sizeof(val));
		return *this;
	}

	SerialBufferIO& operator>>(int& val) {
		_pbuf->GetData(&val, sizeof(val));
		return *this;
	}

	SerialBufferIO& operator>>(unsigned int& val) {
		_pbuf->GetData(&val, sizeof(val));
		return *this;
	}

	SerialBufferIO& operator>>(long& val) {
		_pbuf->GetData(&val, sizeof(val));
		return *this;
	}

	SerialBufferIO& operator>>(unsigned long& val) {
		_pbuf->GetData(&val, sizeof(val));
		return *this;
	}

	SerialBufferIO& operator>>(float& val) {
		_pbuf->GetData(&val, sizeof(val));
		return *this;
	}

	SerialBufferIO& operator>>(long long& val) {
		_pbuf->GetData(&val, sizeof(val));
		return *this;
	}

	SerialBufferIO& operator>>(unsigned long long& val) {
		_pbuf->GetData(&val, sizeof(val));
		return *this;
	}

	SerialBufferIO& operator>>(double& val) {
		_pbuf->GetData(&val, sizeof(val));
		return *this;
	}

};