#pragma once
#include <windows.h>
#include "SysPtrUniqueCounter.h"
#include "MemoryPoolTLS.h"

template <typename T>
class LFStack {
#pragma warning( push )
#pragma warning( disable : 26495 )	// ignoring "init 'data' member" warning.
	struct Node {
		T data;
		Node* next = nullptr;
	};
#pragma warning( pop )
	Node* _top = nullptr;
	long _size = 0;
	MemoryPool<Node> nodepool = MemoryPool<Node>();
	SysPtrUniqueCounter ptrcounter = SysPtrUniqueCounter(1);
public:
	~LFStack() {
		T tmp;
		while (_size > 0) {
			pop(&tmp);
		}
	}

	bool push(T data) {
		Node* localTop;
		Node* newnode = nodepool.Alloc();
		Node* uniq_newnode;
		newnode->data = data;
		while (true) {
			localTop = _top;
			newnode->next = localTop;
			uniq_newnode = (Node*)ptrcounter.makePtrUnique(newnode, ptrcounter.extractIdx(localTop) + 1);
			PVOID retaddr = InterlockedCompareExchangePointer((PVOID*)&_top, uniq_newnode, localTop);
			if (retaddr == (PVOID)localTop) { break; }
		}
		InterlockedIncrement(&_size);

		return true;
	}

	bool pop(T* data) {
		Node* localTop;
		Node* realTop;
		Node* newTop;
		long cursize = InterlockedDecrement(&_size);
		if (cursize < 0) { 
			InterlockedIncrement(&_size);
			return false; 
		}
		while (true) {
			localTop = _top;
			realTop = (Node*)ptrcounter.makeNormalPtr(localTop);
			newTop = realTop->next;
			newTop = (Node*)ptrcounter.makePtrUnique(ptrcounter.makeNormalPtr(newTop), ptrcounter.extractIdx(localTop) + 1);
			PVOID retaddr = InterlockedCompareExchangePointer((PVOID*)&_top, newTop, localTop);
			if (retaddr == (PVOID)localTop) { break; }
		}
		*data = realTop->data;
		nodepool.Free(realTop);
		
		return true;
	}

	long getSize() {return _size;}

	long getPoolCap() { return nodepool.getCapacity(); }
	long getPoolUse() { return nodepool.getUseCount(); }
};

