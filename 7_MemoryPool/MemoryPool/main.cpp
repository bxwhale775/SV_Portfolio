#include <iostream>
#include "MemoryPoolTLS.h"

int main() {
    
    //memorypool
    MemoryPool<int> mp;
    int* a = mp.Alloc();
    mp.Free(a);

    //TLS memorypool
    MemoryPoolTLS<int> mptls;
    int* b = mptls.Alloc();
    mptls.Free(b);

    return 0;
}
