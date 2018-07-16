/*
 * Copyright 2018 Robert C. Curtis. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *    2. Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials
 *       provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY ROBERT C. CURTIS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL ROBERT C. CURTIS OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 *
 * The views and conclusions contained in the software and documentation
 * are those of the authors and should not be interpreted as representing
 * official policies, either expressed or implied, of Robert C. Curtis.
 */

/** @file util/queue.h
 * Static queue primitives.
 *
 * This file contains a set of static queue primitives that are common among
 * all of the low level queue implementations. The primitives are inline
 * functions, and will be optimal if size is a constant power of 2.
 */

#ifndef I__QUEUE_H__
	#define I__QUEUE_H__

/** Increment a queue index.
 * @param[in]	idx	Queue index
 * @param[in]	size	Queue size
 * @returns	The new queue index value
 */
static inline size_t qinc(size_t idx, size_t size)
{
	return ((idx + 1) % size);
}

/** Decrement a queue index.
 * @param[in]	idx	Queue index
 * @param[in]	size	Queue size
 * @returns	The new queue index value
 */
static inline size_t qdec(size_t idx, size_t size)
{
	return ((idx - 1) % size);
}

/** Tests if a queue is full.
 * @param[in]	head	Head index
 * @param[in]	tail	Tail index
 * @param[in]	size	Queue size
 */
static inline int qfull(size_t head, size_t tail, size_t size)
{
	size_t next_head = qinc(head, size);
	return (next_head == tail);
}

/** Tests if a queue is empty.
 * @param[in]	head	Head index
 * @param[in]	tail	Tail index
 */
static inline int qempty(size_t head, size_t tail)
{
	return (head == tail);
}

#endif /* I__QUEUE_H__ */
