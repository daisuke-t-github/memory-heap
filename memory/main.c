/**
 * main.c
 *
 * Copyright (C) 2018 daisuke-t.
 *
 * memory heap management code on C language.
 */

#include <stdio.h>

#include "memory.h"



int main(int argc, const char * argv[]) {
	
	void *p, *p2, *p3;

	// heap size.
	uint32_t heap_size_array[MEMORY_HEAP_MAX] = {
		1024 * 1024 * 1,	// 1MB
		1024 * 1024 * 3,	// 3MB
	};

	
	// memory system init.
	if(!MemoryInit(heap_size_array))
	{
		printf("init error.\n");
		return -1;
	}
	
	
	// alloc test (size over error)
	p = MemoryAlloc(0, 1024 * 1024 * 2);
	if(p != NULL)
	{
		printf("alloc test error(size over error)\n");
		return -1;
	}
	
	// alloc test
	p = MemoryAlloc(1, 1024 * 1024 * 2);
	if(p == NULL)
	{
		printf("alloc test error\n");
		return -1;
	}
	printf("heap1 free size %dB\n", MemoryGetFreeSize(1));
	MemoryFree(p);


	// alloc test
	p = MemoryAlloc(0, 1024);
	p2 = MemoryAlloc(0, 1024);
	p3 = MemoryAlloc(0, 1024);
	MemoryFree(p2);
	MemoryFree(p3);
	MemoryFree(p);

	
	// memory system release.
	MemoryRelease();
	
	return 0;
}
