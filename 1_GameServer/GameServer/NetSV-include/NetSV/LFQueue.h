#pragma once
#include <windows.h>
#include "SysPtrUniqueCounter.h"
#include "MemoryPoolTLS.h"

template <typename T>
class LFQueue {
private:
#pragma warning( push )
#pragma warning( disable : 26495)	// ignoring "init 'data' member" warning.
	struct Node {
		T data;
		Node* next = nullptr;
	};
#pragma warning( pop )
	Node* _head = nullptr;        // EQ point
	Node* _tail = nullptr;        // DQ point
	long _size = 0;

	MemoryPool<Node> nodepool = MemoryPool<Node>();
	SysPtrUniqueCounter ptrcounter = SysPtrUniqueCounter(1);
	
public:
	LFQueue() {
		Node* newnode = nodepool.Alloc();
		newnode->next = nullptr;
		_tail = (Node*)ptrcounter.makePtrUnique(newnode, 0);
		_head = _tail;
		return;
	}
	~LFQueue() {
		T tmp;
		for (int i = 0; i < _size; i++) {
			while (true) {
				bool isdone = Dequeue(&tmp);
				if (isdone) { break; }
			}
		}
		nodepool.Free((Node*)ptrcounter.makeNormalPtr(_tail));
		return;
	}
	long getSize() {
		return _size;
	}

	int getPoolCapacity() {
		return nodepool.getCapacity();
	}

	bool Enqueue(T data) {
		Node* newnode = nodepool.Alloc();
		newnode->data = data;
		newnode->next = nullptr;

		Node* localhead;
		Node* reallocalhead;
		Node* nexthead;
		Node* newhead;
		while (true) {
			localhead = _head;
			newhead = (Node*)ptrcounter.makePtrUnique(newnode, ptrcounter.extractIdx(localhead) + 1);
			reallocalhead = (Node*)ptrcounter.makeNormalPtr(localhead);
			nexthead = reallocalhead->next;
			if (nexthead == nullptr) {
				PVOID retaddr = InterlockedCompareExchangePointer((PVOID*)&reallocalhead->next, newhead, nullptr);
				if (retaddr == (PVOID)nullptr) { 
					PVOID retaddr = InterlockedCompareExchangePointer((PVOID*)&_head, newhead, localhead);
					break;
				}
			}
			else {
				InterlockedCompareExchangePointer((PVOID*)&_head, nexthead, localhead);
			}
		}
		InterlockedIncrement(&_size);

		return true;
	}

	bool Dequeue(T* data) {
		Node* localtail;
		Node* reallocaltail;
		Node* localhead;
		Node* reallocalhead;
		Node* nexthead;
		Node* newtail;
		Node* realnewtail;
		if (_size <= 0) { return false; }
		InterlockedDecrement(&_size);
		while (true) {
			localtail = _tail;
			localhead = _head;
			reallocaltail = (Node*)ptrcounter.makeNormalPtr(localtail);
			newtail = reallocaltail->next;
			reallocalhead = (Node*)ptrcounter.makeNormalPtr(localhead);
			nexthead = reallocalhead->next;
			if (reallocalhead == reallocaltail) {
				if (nexthead != nullptr) {
					nexthead = (Node*)ptrcounter.makePtrUnique(nexthead, ptrcounter.extractIdx(localhead) + 1);
					InterlockedCompareExchangePointer((PVOID*)&_head, nexthead, localhead);
					continue;
				}
				else {
					InterlockedIncrement(&_size);
					return false;
				}
			}
			if (newtail == nullptr) {
				InterlockedIncrement(&_size);
				return false;
			}
			realnewtail = (Node*)ptrcounter.makeNormalPtr(newtail);
			*data = realnewtail->data;
			PVOID retaddr = InterlockedCompareExchangePointer((PVOID*)&_tail, newtail, localtail);
			if (retaddr == (PVOID)localtail) { break; }
		}
		nodepool.Free(reallocaltail);

		return true;
	}
};
