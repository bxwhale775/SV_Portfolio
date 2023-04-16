#pragma once
#include <vector>
#include "MemoryPool.h"


template <typename T>
class MemoryPoolTLS {
private:
	struct Chunk;
#pragma warning( push )
#pragma warning( disable : 26495 )	// ignoring "init 'data' member" warning.
	struct PtrExt {
		int identifycode_front = 0;
		T data;
		int identifycode_back = 0;
		Chunk* chunkptr = nullptr;
	};
#pragma warning( pop ) 
	struct Chunk {
		PtrExt* ptrextArr = nullptr;
		long AllocCount = 0;
		long FreeCount = 0;
		bool isNew = true;
		MemoryPoolTLS* mpptr = nullptr;

		friend class MemoryPoolTLS;

		~Chunk() {
			if (ptrextArr != nullptr) {
				if (!mpptr->_isUsingPlacementNew) {
					for (int i = 0; i < mpptr->_chunksize; i++) {
						ptrextArr[i].data.~T();
					}
				}
				delete[] (char*)ptrextArr;
			}
		}
	};
	MemoryPool<Chunk> _chunkpool = MemoryPool<Chunk>(0, false);
	int _chunksize;
	DWORD _tlsidx;
	int _IdentifyCode = 0xffffffff;
	bool _isUsingPlacementNew;
	//unsigned long _UsingCount = 0;

public:
	MemoryPoolTLS(int initBlockNum = 0, int chunksize = 100, bool PlacementNew = false) {
		//init vars
		_chunksize = chunksize;
		_isUsingPlacementNew = PlacementNew;
		_tlsidx = TlsAlloc();

		//gen _IdentifyCode
		_IdentifyCode = rand();
		int temprd = rand();
		_IdentifyCode <<= sizeof(short);
		_IdentifyCode |= temprd;

		//calc init chunk numbers
		int genchunkcnt = 0;
		if (initBlockNum < 0) { initBlockNum = 0; }
		if (initBlockNum > 0) {
			if ((initBlockNum % chunksize) > 0) {
				genchunkcnt = (initBlockNum / chunksize) + 1;
			}
			else {
				genchunkcnt = initBlockNum / chunksize;
			}
		}

		//create init chunks
		if (genchunkcnt > 0) {
			Chunk** tempchunkarr = new Chunk * [genchunkcnt];
			for (int i = 0; i < genchunkcnt; i++) {
				tempchunkarr[i] = _chunkpool.Alloc();
				initChunk(tempchunkarr[i]);
			}
			for (int i = 0; i < genchunkcnt; i++) {
				_chunkpool.Free(tempchunkarr[i]);
			}
			delete[] tempchunkarr;
		}

		return;
	}
	~MemoryPoolTLS() {
		TlsFree(_tlsidx);
	}

	T* Alloc() {
		//get chunk from tls
		Chunk* currentThrChk = (Chunk*)TlsGetValue(_tlsidx);

		//if there was nothing, get one from pool
		if (currentThrChk == nullptr) {
			currentThrChk = _chunkpool.Alloc();
			initChunk(currentThrChk);
			TlsSetValue(_tlsidx, currentThrChk);
		}

		//get data to alloc, and increase alloccnt
		T* ret = &currentThrChk->ptrextArr[currentThrChk->AllocCount].data;
		currentThrChk->AllocCount++;

		//if alloccnt reached to chunksize, remove chunk from tls
		if (currentThrChk->AllocCount == _chunksize) {
			TlsSetValue(_tlsidx, nullptr);
		}

		//call constructor if using placement new
		//				   or not using placement new but chunk is brand-new
		if (_isUsingPlacementNew || (!_isUsingPlacementNew && currentThrChk->isNew)) {
			ret = new (ret) T();
		}
		//InterlockedIncrement(&_UsingCount);
		return ret;
	}
	bool Free(T* pdata) {
		//get PtrExt from pdata and verify it
		PtrExt* fptrext = dataToPtrExt(pdata);
		if (fptrext->identifycode_front != _IdentifyCode ||
			fptrext->identifycode_back != _IdentifyCode) {
			return false;
		}
		//InterlockedDecrement(&_UsingCount);
		//if using placement new, call destructor
		if (_isUsingPlacementNew) {
			fptrext->data.~T();
		}

		//increase freecount - using interlocked because it can be used by other thread
		//if cnt reached to chunksize, free the chunk
		long increasedFCnt = InterlockedIncrement(&fptrext->chunkptr->FreeCount);
		if (increasedFCnt == _chunksize) {
			fptrext->chunkptr->isNew = false;
			_chunkpool.Free(fptrext->chunkptr);
		}

		return true;
	}

	int getCapacity() { return _chunkpool.getCapacity() * _chunksize; }
	int getUseCount() { return _chunkpool.getUseCount() * _chunksize; }
private:
	void initChunk(Chunk* chk) {
		//if chunk is brand-new, do init
		if (chk->isNew) {
			//alloc memory for PtrExt Array, and init metadata
			chk->ptrextArr = (PtrExt*)new char[sizeof(PtrExt) * _chunksize];
			for (int i = 0; i < _chunksize; i++) {
				chk->ptrextArr[i].identifycode_front = _IdentifyCode;
				chk->ptrextArr[i].identifycode_back = _IdentifyCode;
				chk->ptrextArr[i].chunkptr = chk;
			}

			chk->mpptr = this;
		}

		//init var;
		chk->AllocCount = 0;
		chk->FreeCount = 0;
		return;
	}
	PtrExt* dataToPtrExt(T* data) {
		size_t offset = offsetof(PtrExt, data);
		return (PtrExt*)((char*)data - offset);
	}
};
