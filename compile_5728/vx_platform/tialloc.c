/*
 *  Copyright (C) 2007-2014 Texas Instruments Incorporated - http://www.ti.com
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>

#define HEAP_ALIGN 64
typedef unsigned long phys_addr_t;
/*
 * Change here for supporting more than 4 blocks.  Also change all
 * NBLOCKS-based arrays to have NBLOCKS-worth of initialization values.
 */
#define NBLOCKS 4

/*
 *  NOTE: The following implementation of a heap is taken from the
 *  DSP/BIOS 6.0 source tree (avala-f15/src/ti/sysbios/heaps).  Changes
 *  are necessary due to the fact that CMEM is not built in an XDC
 *  build environment, and therefore XDC types and helper APIs (e.g.,
 *  Assert) are not available.  However, these changes were kept to a
 *  minimum.
 *
 *  The changes include:
 *	- renaming XDC types to standard C types
 *	- replacing sizeof(HeapMem_Header) w/ HEAP_ALIGN throughout
 *
 *  As merged with CMEM, the heap becomes a distinguished "pool" and
 *  is sometimes treated specially, and at other times can be treated
 *  as a normal pool instance.
 */

/*
 * HeapMem compatibility stuff
 */
typedef struct HeapMem_Header {
    phys_addr_t next;
    size_t size;
} HeapMem_Header;

phys_addr_t HeapMem_alloc(int bi, size_t size, size_t align);
void HeapMem_free(int bi, phys_addr_t block, size_t size);

/*
 * Heap configuration stuff
 *
 * For CMA global heap allocations, we treat heap_pool[NBLOCKS] as
 * its own block.  For example, if you have 4 physically-specified
 * blocks then NBLOCKS = 4.  heap_pool[0]|[1]|[2]|[3] are the real blocks, and
 * heap_pool[4] represents the global CMA area.
 *
 * Only heap_pool[] gets extended with NBLOCKS + 1 dimension, since the
 * other heap_*[] arrays are used only with the real blocks.  You can't
 * use heap_pool[NBLOCKS] for HeapMem_alloc().
 */
static HeapMem_Header heap_head[NBLOCKS] = {
    {
	0,	/* next */
	0	/* size */
    },
    {
	0,	/* next */
	0	/* size */
    },
    {
	0,	/* next */
	0	/* size */
    },
/* cut-and-paste below as part of adding support for more than 4 blocks */
    {
	0,	/* next */
	0	/* size */
    },
/* cut-and-paste above as part of adding support for more than 4 blocks */
};

/*
 *  ======== HeapMem_alloc ========
 *  HeapMem is implemented such that all of the memory and blocks it works
 *  with have an alignment that is a multiple of HEAP_ALIGN and have a size
 *  which is a multiple of HEAP_ALIGN. Maintaining this requirement
 *  throughout the implementation ensures that there are never any odd
 *  alignments or odd block sizes to deal with.
 *
 *  Specifically:
 *  The buffer managed by HeapMem:
 *    1. Is aligned on a multiple of HEAP_ALIGN
 *    2. Has an adjusted size that is a multiple of HEAP_ALIGN
 *  All blocks on the freelist:
 *    1. Are aligned on a multiple of HEAP_ALIGN
 *    2. Have a size that is a multiple of HEAP_ALIGN
 *  All allocated blocks:
 *    1. Are aligned on a multiple of HEAP_ALIGN
 *    2. Have a size that is a multiple of HEAP_ALIGN
 *
 */
phys_addr_t HeapMem_alloc(int bi, size_t reqSize, size_t reqAlign)
{
    HeapMem_Header *curHeader;
    HeapMem_Header *prevHeader;
    HeapMem_Header *newHeader;
    phys_addr_t curHeaderPhys;
    phys_addr_t prevHeaderPhys = 0;
    phys_addr_t newHeaderPhys = 0;  /* init to quiet compiler */
    phys_addr_t allocAddr;
    size_t curSize, adjSize;
    size_t remainSize;  /* free memory after allocated memory */
    size_t adjAlign, offset;

    adjSize = reqSize;

    /* Make size requested a multiple of HEAP_ALIGN */
    if ((offset = (adjSize & (HEAP_ALIGN - 1))) != 0) {
        adjSize = adjSize + (HEAP_ALIGN - offset);
    }

    /*
     *  Make sure the alignment is at least as large as HEAP_ALIGN.
     *  Note: adjAlign must be a power of 2 (by function constraint) and
     *  HEAP_ALIGN is also a power of 2,
     */
    adjAlign = reqAlign;
    if (adjAlign & (HEAP_ALIGN - 1)) {
        /* adjAlign is less than HEAP_ALIGN */
        adjAlign = HEAP_ALIGN;
    }

    /*
     * The block will be allocated from curHeader. Maintain a pointer to
     * prevHeader so prevHeader->next can be updated after the alloc.
     */
    curHeaderPhys = heap_head[bi].next;

    /* Loop over the free list. */
    while (curHeaderPhys != 0) {
        curHeader = (HeapMem_Header *)curHeaderPhys;
        curSize = curHeader->size;

        /*
         *  Determine the offset from the beginning to make sure
         *  the alignment request is honored.
         */
        offset = (unsigned long)curHeaderPhys & (adjAlign - 1);
        if (offset) {
            offset = adjAlign - offset;
        }

        /* big enough? */
        if (curSize >= (adjSize + offset)) {
            /* Set the pointer that will be returned. Alloc from front */
            allocAddr = curHeaderPhys + offset;

            /*
             *  Determine the remaining memory after the allocated block.
             *  Note: this cannot be negative because of above comparison.
             */
            remainSize = curSize - adjSize - offset;

	    if (remainSize) {
		newHeaderPhys = allocAddr + adjSize;
                newHeader = (HeapMem_Header *)newHeaderPhys;
		newHeader->next = curHeader->next;
		newHeader->size = remainSize;
	    }

            /*
             *  If there is memory at the beginning (due to alignment
             *  requirements), maintain it in the list.
             *
             *  offset and remainSize must be multiples of
             *  HEAP_ALIGN. Therefore the address of the newHeader
             *  below must be a multiple of the HEAP_ALIGN, thus
             *  maintaining the requirement.
             */
            if (offset) {
                /* Adjust the curHeader size accordingly */
                curHeader->size = offset;

                /*
                 *  If there is remaining memory, add into the free list.
                 *  Note: no need to coalesce and we have HeapMem locked so
                 *        it is safe.
                 */
                if (remainSize) {
                    curHeader->next = newHeaderPhys;
                }
            }
            else {
                /*
                 *  If there is any remaining, link it in,
                 *  else point to the next free block.
                 *  Note: no need to coalesce and we have HeapMem locked so
                 *        it is safe.
                 */
		if (prevHeaderPhys != 0) {
                    prevHeader = (HeapMem_Header *)prevHeaderPhys;
		}
		else {
		    prevHeader = &heap_head[bi];
		}

                if (remainSize) {
                    prevHeader->next = newHeaderPhys;
                }
                else {
                    prevHeader->next = curHeader->next;
                }
            }

            /* Success, return the allocated memory */
            return allocAddr;
        }
        else {
	    prevHeaderPhys = curHeaderPhys;
	    curHeaderPhys = curHeader->next;
        }
    }

    return 0;
}

/*
 *  ======== HeapMem_free ========
 */
void HeapMem_free(int bi, phys_addr_t block, size_t size)
{
    HeapMem_Header *curHeader;
    HeapMem_Header *newHeader;
    HeapMem_Header *nextHeader;
    phys_addr_t curHeaderPhys = 0;
    phys_addr_t newHeaderPhys;
    phys_addr_t nextHeaderPhys;
    size_t offset;

    /* Restore size to actual allocated size */
    if ((offset = size & (HEAP_ALIGN - 1)) != 0) {
        size += HEAP_ALIGN - offset;
    }

    newHeaderPhys = block;
    nextHeaderPhys = heap_head[bi].next;

    /* Go down freelist and find right place for buf */
    while (nextHeaderPhys != 0 && nextHeaderPhys < newHeaderPhys) {
        nextHeader = (HeapMem_Header *)nextHeaderPhys;
        curHeaderPhys = nextHeaderPhys;
        nextHeaderPhys = nextHeader->next;
    }
    newHeader = (HeapMem_Header *)newHeaderPhys;

    if (curHeaderPhys != 0) {
        curHeader = (HeapMem_Header *)curHeaderPhys;
    }
    else {
	curHeader = &heap_head[bi];
    }

    newHeader->next = nextHeaderPhys;
    newHeader->size = size;
    curHeader->next = newHeaderPhys;

    /* Join contiguous free blocks */
    /* Join with upper block */
    if (nextHeaderPhys != 0 && (newHeaderPhys + size) == nextHeaderPhys) {
        nextHeader = (HeapMem_Header *)nextHeaderPhys;

        newHeader->next = nextHeader->next;
        newHeader->size += nextHeader->size;
    }

    /*
     * Join with lower block. Make sure to check to see if not the
     * first block.
     */
    if (curHeader != &heap_head[bi]) {
        if ((curHeaderPhys + curHeader->size) == newHeaderPhys) {
	    curHeader->next = newHeader->next;
	    curHeader->size += newHeader->size;
	}
    }
}
/*
 *  ======== HeapMem_init ========
 */
void HeapMem_init (int bi, phys_addr_t block_base, size_t size)
{
   HeapMem_Header *hdr = (HeapMem_Header *)block_base;
   heap_head[bi].next = (phys_addr_t)hdr;
   hdr->next = 0;
   hdr->size = size;
   memset ((char *)block_base, size, 0);
}

/* =============================== */
