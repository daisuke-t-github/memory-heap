/**
 * memory.c
 *
 * Copyright (C) 2018 daisuke-t.
 */

#include "memory.h"

#include <string.h>
#include <stdlib.h>



// memory control block.
typedef struct
{
	void *pre_cb;
	void *next_cb;
	uint32_t size;	// allocate size(without CB size)
} MemoryCB;

// work.
typedef struct
{
	void *heap[MEMORY_HEAP_MAX];
	uint32_t heap_size[MEMORY_HEAP_MAX];
	MemoryCB *cb[MEMORY_HEAP_MAX];	// heap head cb.
} Work;
static Work _work;

static void * MemoryAllocInner(uint8_t heap, uint32_t size);
static void * MemoryAllocInnerHeapNotUsed(uint8_t heap, uint32_t size);
static void * MemoryAllocInnerHeapHead(uint8_t heap, uint32_t size);
static void * MemoryAllocInnerHeapBetween(uint8_t heap, uint32_t size);
static void * MemoryGetHeapEndAddress(uint8_t heap);



bool MemoryInit(uint32_t *heap_size_array)
{
	uint32_t i;

	// clear work.
	memset(&_work, 0, sizeof(Work));

	printf("memory system init...\n");
	printf("cb size %dB\n", (int)sizeof(MemoryCB));
	printf("heap0 %dKB\n", heap_size_array[0] / 1024);
	printf("heap1 %dKB\n", heap_size_array[1] / 1024);

	
	// alloc heap.
	for(i = 0; i < MEMORY_HEAP_MAX; i++)
	{
		_work.heap[i] = malloc(heap_size_array[i]);
		_work.heap_size[i] = heap_size_array[i];

		if(_work.heap[i] == NULL)
		{
			// heap alloc error.
			printf("heap %d alloc error!", i);
			return false;
		}
	}

	return true;
}

void MemoryRelease(void)
{
	uint32_t i;

	for(i = 0; i < MEMORY_HEAP_MAX; i++)
	{
		if(_work.heap[i] == NULL) continue;

		free(_work.heap[i]);
		_work.heap[i] = NULL;
	}
}



void * MemoryAlloc(uint8_t heap, uint32_t size)
{
	void *p;

	// check invalid arguments.
	if(size <= 0) return NULL;
	if(heap < 0 || heap >= MEMORY_HEAP_MAX) return NULL;


	p = MemoryAllocInner(heap, size);

	if(p == NULL)
	{
		printf("alloc error heap[%d] size[%d byte]\n", heap, size);
	}

	return p;
}

static void * MemoryAllocInner(uint8_t heap, uint32_t size)
{
	uint32_t size2, free_size;
	void *p;

	// need alloc size = cb size + request size.
	size2 = sizeof(MemoryCB) + size;

	if(_work.cb[heap] == NULL)
	{
		/**
		 * alloc for not used heap.
		 */
		if(size2 > _work.heap_size[heap]) return NULL;	// over heap size.
		
		p = MemoryAllocInnerHeapNotUsed(heap, size);

		return p;
	}

	// free size on heap head.
	free_size = (uint32_t)((intptr_t)_work.cb[heap] - (intptr_t)_work.heap[heap]);
	if(size2 <= free_size)
	{
		/**
		 * alloc for heap head.
		 */
		p = MemoryAllocInnerHeapHead(heap, size);
		
		return p;
	}

	
	/**
	 * alloc for cb bewteen.
	 */
	p = MemoryAllocInnerHeapBetween(heap, size);

	return p;
}

static void * MemoryAllocInnerHeapNotUsed(uint8_t heap, uint32_t size)
{
	MemoryCB *cb;
	void *p;

	// new control block.
	cb = (MemoryCB *)_work.heap[heap];
	cb->pre_cb = NULL;
	cb->next_cb = NULL;
	cb->size = size;

	// set head cb on heap.
	_work.cb[heap] = cb;

	// allocated addr = cb addr + cb size
	p = (void *)((intptr_t)cb + sizeof(MemoryCB));

	return p;
}

static void * MemoryAllocInnerHeapHead(uint8_t heap, uint32_t size)
{
	MemoryCB *cb;
	void *p;

	// new control block.
	cb = (MemoryCB *)_work.heap[heap];
	cb->pre_cb = NULL;
	cb->next_cb = _work.cb[heap];	// next cb is current head cb.
	cb->size = size;

	// head cb setting.
	_work.cb[heap]->pre_cb = cb;	// set current head cb's pre.
	_work.cb[heap] = cb;
	
	// allocated addr = cb addr + cb size
	p = (void *)((intptr_t)cb + sizeof(MemoryCB));

	return p;
}

static void * MemoryAllocInnerHeapBetween(uint8_t heap, uint32_t size)
{
	MemoryCB *cb, *cb_next, *cb_new;
	void *p, *p_end;
	size_t size2, free_size;

	size2 = sizeof(MemoryCB) + size;

	// heap head cb.
	cb = _work.cb[heap];

	while(1)
	{
		// next cb.
		cb_next = cb->next_cb;
		
		if(cb_next == NULL)
		{
			/**
			 * tail cb.
			 */
			// cb's allocated addr end.
			p_end = (void *)((intptr_t)cb + sizeof(MemoryCB) + cb->size);
			
			// tail free size.
			free_size = (intptr_t)MemoryGetHeapEndAddress(heap) - (intptr_t)p_end;
			
			if(size2 <= free_size)
			{
				/**
				 * alloc for tail.
				 */
				// new control block.
				cb_new = (MemoryCB *)p_end;	// new cb addr = cb addr + cb size + allocated size.
				cb_new->pre_cb = cb;
				cb_new->next_cb = NULL;
				cb_new->size = size;
				
				cb->next_cb = cb_new;
				
				// allocated addr = cb addr + cb size
				p = (void *)((intptr_t)cb_new + sizeof(MemoryCB));
				
				return p;
			}
			
			// can not alloc for tail space.
			return NULL;
		}
		
		
		/**
		 * alloc for between.
		 */
		// cb's allocated addr end.
		p_end = (void *)((intptr_t)cb + sizeof(MemoryCB) + cb->size);

		// free size.
		free_size = (intptr_t)cb_next - (intptr_t)p_end;
		if(size2 <= free_size)
		{
			/**
			 * alloc for between.
			 */
			// new control block.
			cb_new = (MemoryCB *)p_end;	// new cb addr = cb addr + cb size + allocated size.
			cb_new->pre_cb = cb;
			cb_new->next_cb = cb_next;
			cb_new->size = size;
			
			cb->next_cb = cb_new;
			cb_next->pre_cb = cb_new;
			
			// allocated addr = cb addr + cb size
			p = (void *)((intptr_t)cb_new + sizeof(MemoryCB));
		
			return p;
		}
		
		// step cb link.
		cb = cb_next;
	}
	
	return NULL;
}



void MemoryFree(void *p)
{
	uint8_t heap;
	MemoryCB *cb;

	// check invalid arguments.
	if(p == NULL) return;

	// heap of contains addr.
	heap = MemoryGetAllocatedHeap(p);

	if(heap < 0)
	{
		printf("free error %p is not managed.\n", p);
		return;
	}

	// heap head cb.
	cb = _work.cb[heap];

	while(1)
	{
		if(cb == NULL) break;	// cb link end.

		if((intptr_t)p == (intptr_t)cb + sizeof(MemoryCB))
		{
			/**
			 * target cb.
			 */
			if(cb->pre_cb != NULL)
			{
				// remove next cb link for pre cb.
				((MemoryCB *)cb->pre_cb)->next_cb = cb->next_cb;
			}
			if(cb->next_cb != NULL)
			{
				// remove pre cb link for next cb.
				((MemoryCB *)cb->next_cb)->pre_cb = cb->pre_cb;
			}

			if(cb == _work.cb[heap])
			{
				// set new cb if remove heap head cb.
				_work.cb[heap] = cb->next_cb;
			}

			break;
		}

		// step cb link.
		cb = cb->next_cb;
	}
}



uint32_t MemoryGetHeapSize(uint8_t heap)
{
	return _work.heap_size[heap];
}

static void * MemoryGetHeapEndAddress(uint8_t heap)
{
	return (void *)((intptr_t)_work.heap[heap] + _work.heap_size[heap]);
}

/**
 * heap of contains addr.
 */
uint8_t	MemoryGetAllocatedHeap(void *p)
{
	uint32_t i;

	for(i = 0; i < MEMORY_HEAP_MAX; i++)
	{
		if((intptr_t)p >= (intptr_t)_work.heap[i] &&
			(intptr_t)p < (intptr_t)MemoryGetHeapEndAddress(i))
			return i;
	}

	return -1;
}

uint32_t MemoryGetAllocatedSize(uint8_t heap)
{
	uint32_t size;
	MemoryCB *cb;

	cb = _work.cb[heap];
	size = 0;

	while(1)
	{
		if(cb == NULL) break;

		size += sizeof(MemoryCB) + cb->size;

		cb = cb->next_cb;
	}

	return size;
}

uint32_t MemoryGetFreeSize(uint8_t heap)
{
	uint32_t size;

	size = MemoryGetAllocatedSize(heap);

	return _work.heap_size[heap] - size;
}



uint32_t MemoryGetCBNum(uint8_t heap)
{
	MemoryCB *cb;
	uint32_t num;

	cb = _work.cb[heap];
	num = 0;

	while(1)
	{
		if(cb == NULL) break;

		num++;

		cb = cb->next_cb;
	}

	return num;
}
