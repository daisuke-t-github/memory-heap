/**
 * memory.h
 *
 * Copyright (C) 2018 tetsugaku.info
 */

#ifndef	___MEMORY_H_
#define	___MEMORY_H_

#include <stdio.h>
#include <stdbool.h>


#define MEMORY_HEAP_MAX	2



// initial/release
extern bool MemoryInit(uint32_t *heap_size_array);
extern void MemoryRelease(void);

// alloc/free
extern void * MemoryAlloc(uint8_t heap, uint32_t size);
extern void MemoryFree(void *p);

// heap
extern uint32_t MemoryGetHeapSize(uint8_t heap);
extern uint32_t MemoryGetAllocatedSize(uint8_t heap);
extern uint32_t MemoryGetFreeSize(uint8_t heap);
extern uint32_t MemoryGetCBNum(uint8_t heap);
extern uint8_t	MemoryGetAllocatedHeap(void *p);



#endif	// ___MEMORY_H_


