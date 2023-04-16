#ifndef RINGBUFFER_H
#define RINGBUFFER_H

class CRingBuffer {
private:
	int _head = 0;
	int _tail = 0;
	int _size;
	char* _buf;
	SRWLOCK _enqLock;
	SRWLOCK _deqLock;

	int clamp(int p) {
		return p % _size;
	}
public:
	CRingBuffer();
	CRingBuffer(int size);
	~CRingBuffer();
	int GetBufferSize();
	int GetUseSize();
	int GetFreeSize();
	bool isFull();
	bool isEmpty();
	int DirectEnqueueSize();
	int DirectDequeueSize();
	void GetSizeForWSARecv(unsigned long* first, unsigned long* second);
	int Enqueue(char* chpData, int iSize);
	int Dequeue(char* chpDest, int iSize);
	_Acquires_exclusive_lock_(_enqLock)
	void LockEnqueue();
	_Requires_lock_held_(_enqLock)
	void UnlockEnqueue();
	_Acquires_exclusive_lock_(_deqLock)
	void LockDequeue();
	_Requires_lock_held_(_deqLock)
	void UnlockDequeue();
	int Peek(char* chpDest, int iSize);
	void MoveHead(int mov);
	void MoveTail(int mov);
	void ClearBuffer();
	char* GetHeadBufferPtr();
	char* GetTailBufferPtr();
	char* GetBufferPtr();
};

#endif
