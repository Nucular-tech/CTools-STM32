/*
 * simplequeue.h
 *
 *  Created on: 20 џэт. 2021 у.
 *      Author: VasiliSk
 */

#pragma once

#define SimpleQ_GetSize( in,  out,  size) 	((in >= out) ? ( in - out) : ( in + (size - out)))

#define SimpleQ_IsFull( in,  out,  size)  (in == ((out - 1 + size) % size))

#define SimpleQ_HaveSpace(in,  out,  size, space)  (space <= (size-SimpleQ_GetSize(in,  out,  size)))

#define SimpleQ_InSizeTillEnd( in,  out,  size) ((in >= out) ? (size-in) : (out-in))

#define SimpleQ_IsEmpty(in, out) (in == out)

//fifo[in] = object;
#define SimpleQ_AdedOne(in, size)	in = (in + 1) % size
#define SimpleQ_Aded( in, size, added)	in = (in + added) % size

//	fifo[out];
#define SimpleQ_GotOne( out, size)  out = (out + 1) % size;
#define SimpleQ_Got( out, size, got) out = (out + got) % size;
