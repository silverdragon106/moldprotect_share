#ifndef TIALLOC_H
#define TIALLOC_H
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

#define HEAP_ALIGN 64
typedef unsigned long phys_addr_t;

phys_addr_t HeapMem_alloc(int bi, size_t reqSize, size_t reqAlign);
void HeapMem_free(int bi, phys_addr_t block, size_t size);
void HeapMem_init (int bi, phys_addr_t block_base, size_t size);
#endif
