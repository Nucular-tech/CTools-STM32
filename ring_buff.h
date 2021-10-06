#include <stdint.h>
#pragma once

/*
 * Ring buffer implementation helpers
 */

/* Wrap up buffer index */
static inline uint32_t ring_wrap(uint32_t size, uint32_t idx)
{
	return idx >= size ? idx - size : idx;
}

/* Returns the number of bytes available in buffer */
static inline uint32_t ring_data_avail(uint32_t size, uint32_t head, uint32_t tail)
{
	if (head >= tail)
		return head - tail;
	else
		return size + head - tail;
}

/* Returns the amount of free space available in buffer */
static inline uint32_t ring_space_avail(uint32_t size, uint32_t head, uint32_t tail)
{
	return size - ring_data_avail(size, head, tail) - 1;
}

/* Returns the number of contiguous data bytes available in buffer */
static inline uint32_t ring_data_contig(uint32_t size, uint32_t head, uint32_t tail)
{
	if (head >= tail)
		return head - tail;
	else
		return size - tail;
}

/* Returns the amount of contiguous space available in buffer */
static inline uint32_t ring_space_contig(uint32_t size, uint32_t head, uint32_t tail)
{
	if (head >= tail)
		return (tail ? size : size - 1) - head;
	else
		return tail - head - 1;
}

/* Returns the amount of free space available after wrapping up the head */
static inline uint32_t ring_space_wrapped(uint32_t size, uint32_t head, uint32_t tail)
{
	(void) size;
	if (head < tail || !tail)
		return 0;
	else
		return tail - 1;
}

#define MIN_(a, b) (a) < (b) ? (a) : (b)
#define MAX_(a, b) (a) > (b) ? (a) : (b)
