#pragma once
#include <Windows.h>
#include "SysPtrUniqueCounter.h"

template <typename T>
class MemoryPool {
private:
#pragma warning( push )
#pragma warning( disable : 26495)	// ignoring "init 'data' member" warning.
	struct Block {
		int identifycode_front = 0;
		T data;
		int identifycode_back = 0;
		ULONG64 AllocCnt = 0;
		Block* next = nullptr;
	};
#pragma warning( pop ) 
	Block* _pLastFreedBlock = nullptr;
	int _IdentifyCode = 0xffffffff;
	bool _isUsingPlacementNew = false;
	long _Capacity = 0;
	long _UsingBlockNum = 0;
	SysPtrUniqueCounter ptrcounter = SysPtrUniqueCounter(1);
public:
	MemoryPool(int initBlockNum = 0, bool PlacementNew = false) {
		_isUsingPlacementNew = PlacementNew;
		_IdentifyCode = rand();
		int temprd = rand();
		_IdentifyCode <<= sizeof(short);
		_IdentifyCode |= temprd;
		if (initBlockNum < 0) { initBlockNum = 0; }
		for (int i = 0; i < initBlockNum; i++) {
			createNewBlock();
		}
	}
	virtual ~MemoryPool() {
		if (_Capacity == 0) { return; }
		Block* nextholder;
		auto i = _pLastFreedBlock;
		i = (Block*)ptrcounter.makeNormalPtr(i);
		if (i == nullptr) { return; }
		if (_isUsingPlacementNew) {
			while (true) {
				nextholder = i->next;
				nextholder = (Block*)ptrcounter.makeNormalPtr(nextholder);
				free(i);
				if (nextholder == nullptr) { break; }
				i = nextholder;
			}
		}
		else {
			while (true) {
				nextholder = i->next;
				nextholder = (Block*)ptrcounter.makeNormalPtr(nextholder);
				delete i;
				if (nextholder == nullptr) { break; }
				i = nextholder;
			}
		}
	}

	T* Alloc() {
		if(_pLastFreedBlock == nullptr){ createNewBlock(); }
		Block* localLFB;
		Block* reallocalLFB;
		Block* newLFB;
		T* ret;
		InterlockedIncrement(&_UsingBlockNum);
		while (true) {
			localLFB = _pLastFreedBlock;
			if (localLFB == nullptr) { createNewBlock(); continue; }
			reallocalLFB = (Block*)ptrcounter.makeNormalPtr(localLFB);
			ret = &(reallocalLFB->data);
			newLFB = reallocalLFB->next;
			PVOID retaddr = InterlockedCompareExchangePointer((PVOID*)&_pLastFreedBlock, newLFB, localLFB);
			if (retaddr == (PVOID)localLFB) { break; }
		}
		reallocalLFB->AllocCnt++;
		if (_isUsingPlacementNew) {
			ret = new (ret) T();
			return ret;
		}
		else {
			return ret;
		}
	}
	bool Free(T* pBlock) {
		Block* fblock = DataToBlock(pBlock);
		if (fblock->identifycode_front != _IdentifyCode || 
			fblock->identifycode_back != _IdentifyCode) {
			return false;
		}
		if (_isUsingPlacementNew) {
			fblock->data.~T();
		}
		Block* localLFB;
		Block* newLFB;
		Block* uniquefblock = (Block*)ptrcounter.makePtrUnique(fblock, fblock->AllocCnt + 1);
		newLFB = uniquefblock;
		while (true) {
			localLFB = _pLastFreedBlock;
			fblock->next = localLFB;
			PVOID retaddr = InterlockedCompareExchangePointer((PVOID*)&_pLastFreedBlock, newLFB, localLFB);
			if (retaddr == (PVOID)localLFB) { break; }
		}
		InterlockedDecrement(&_UsingBlockNum);
		return true;
	}

	int getCapacity() { return _Capacity; }
	int getUseCount() { return _UsingBlockNum; }
private:
#pragma warning( push )
#pragma warning( disable : 6011)	//ignoring 'deref nullptr error' caused by malloc failure. 
	void createNewBlock() {
		Block* newblock;
		if (_isUsingPlacementNew) { newblock = (Block*)malloc(sizeof(Block)); }
		else { newblock = new Block(); }
		newblock->identifycode_back = _IdentifyCode;
		newblock->identifycode_front = _IdentifyCode;
		newblock->next = nullptr;
		Block* localLFB;
		Block* uniquenewblock;
		Block* newLFB;
		uniquenewblock = (Block*)ptrcounter.makePtrUnique(newblock, 0);
		newLFB = uniquenewblock;
		while (true) {
			localLFB = _pLastFreedBlock;
			newblock->next = localLFB;
			PVOID retaddr = InterlockedCompareExchangePointer((PVOID*)&_pLastFreedBlock, newLFB, localLFB);
			if (retaddr == (PVOID)localLFB) { break; }
		}
		InterlockedIncrement(&_Capacity);
		return;
	}
#pragma warning( pop ) 
	Block* DataToBlock(T* data) {
		size_t offset = offsetof(Block, data);
		return (Block*)((char*)data - offset);
	}
	T* BlockToData(Block* block) {
		return &(block->data);
	}
};

