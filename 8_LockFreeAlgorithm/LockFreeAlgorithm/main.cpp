#include <iostream>
#include "LFStack.h"
#include "LFQueue.h"

int main() {
	int a = 0;

	//lock free stack
	LFStack<int> lfs;
	lfs.push(1);
	lfs.pop(&a);

	//lock free queue
	LFQueue<int> lfq;
	lfq.Enqueue(2);
	lfq.Dequeue(&a);

	return 0;
}
