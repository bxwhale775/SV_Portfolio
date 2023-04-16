#include <stdlib.h>
#include <memory.h>
#include <windows.h>
#include "NetSV-include/NetSV/ringbuffer.h"

CRingBuffer::CRingBuffer() {
	InitializeSRWLock(&_enqLock);
	InitializeSRWLock(&_deqLock);
	_buf = nullptr;
	_size = 0;
	return;
}

CRingBuffer::CRingBuffer(int size) : _size(size+1) {
	InitializeSRWLock(&_enqLock);
	InitializeSRWLock(&_deqLock);
	_buf = new char[_size];
	return;
}

CRingBuffer::~CRingBuffer() {
	if(_buf != nullptr){ delete[] _buf; }
	return;
}

int CRingBuffer::GetBufferSize() {
	return _size - 1;
}

int CRingBuffer::GetUseSize() {
	if (_head >= _tail) {
		return _head - _tail;
	}
	else {
		return _head + _size - _tail;
	}
}

int CRingBuffer::GetFreeSize() {
	return GetBufferSize() - GetUseSize();
}

bool CRingBuffer::isFull() {
	return clamp(_head + 1) == _tail;
}

bool CRingBuffer::isEmpty() {
	return _head == _tail;
}

int CRingBuffer::DirectEnqueueSize() {
	if (_tail > _head) {
		return  (_tail - 1) - _head;
	}
	else {
		return GetBufferSize() - _head + 1;
	}
}

int CRingBuffer::DirectDequeueSize() {
	if (_head >= _tail) {
		return _head - _tail;
	}
	else {
		return (GetBufferSize() + 1) - _tail;
	}
}

void CRingBuffer::GetSizeForWSARecv(unsigned long* first, unsigned long* second) {
	int maxsize = GetBufferSize();
	if (_tail <= _head) {
		*first = maxsize - _head + 1;
		*second = _tail;
		if (*second == 0) { *first -= 1; }
		else { *second -= 1; }
	}
	else {
		*first = _tail - _head - 1;
		*second = 0;
	}
	return;
}

int CRingBuffer::Enqueue(char* chpData, int iSize) {
	int eqsize = 0;
	for (int i = 0; i < iSize; i++) {
		if (eqsize == iSize || isFull()) {
			break;
		}
		_buf[_head] = chpData[i];
		MoveHead(1);
		eqsize++;
	}
	return eqsize;
}

int CRingBuffer::Dequeue(char* chpDest, int iSize) {
	int dqsize = 0;
	int ddqsize = DirectDequeueSize();
	int dqcount = min(ddqsize, iSize);
	memcpy(chpDest, &_buf[_tail], dqcount);
	MoveTail(dqcount);
	dqsize += dqcount;
	iSize -= dqcount;
	if (iSize > 0 && !isEmpty()) {
		ddqsize = DirectDequeueSize();
		dqcount = min(ddqsize, iSize);
		memcpy(&chpDest[dqsize], &_buf[_tail], dqcount);
		MoveTail(dqcount);
		dqsize += dqcount;
	}
	return dqsize;
}

_Acquires_exclusive_lock_(_enqLock)
void CRingBuffer::LockEnqueue() {
	AcquireSRWLockExclusive(&_enqLock);
}

_Requires_lock_held_(_enqLock)
void CRingBuffer::UnlockEnqueue() {
	ReleaseSRWLockExclusive(&_enqLock);
}
_Acquires_exclusive_lock_(_deqLock)
void CRingBuffer::LockDequeue() {
	AcquireSRWLockExclusive(&_deqLock);
}

_Requires_lock_held_(_deqLock)
void CRingBuffer::UnlockDequeue() {
	ReleaseSRWLockExclusive(&_deqLock);
}

int CRingBuffer::Peek(char* chpDest, int iSize) {
	int prevtail = _tail;
	int dqsize = Dequeue(chpDest, iSize);
	_tail = prevtail;
	return dqsize;
}

void CRingBuffer::MoveHead(int mov) {
	_head = clamp(_head + mov);
	return;
}

void CRingBuffer::MoveTail(int mov) {
	_tail = clamp(_tail + mov);
	return;
}

void CRingBuffer::ClearBuffer() {
	_head = 0;
	_tail = 0;
	return;
}

char* CRingBuffer::GetHeadBufferPtr() {
	return &_buf[_head];
}

char* CRingBuffer::GetTailBufferPtr() {
	return &_buf[_tail];
}

char* CRingBuffer::GetBufferPtr() {
	return _buf;
}
