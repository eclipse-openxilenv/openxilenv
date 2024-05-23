/*
 * Copyright 2023 ZF Friedrichshafen AG
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#include <string.h>

#include "Config.h"
#include "MyMemory.h"
#include "StringMaxChar.h"
#include "Message.h"
#include "Fifos.h"

#include "RealtimeScheduler.h"

#include "RemoteMasterLock.h"

#define InitializeCriticalSection(x) RemoteMasterInitLock(x)
#define DeleteCriticalSection(x) RemoteMasterDestroyLock(x)
#define EnterCriticalSection(x)  RemoteMasterLock(x, __LINE__, __FILE__)
#define LeaveCriticalSection(x)  RemoteMasterUnlock(x)


MESSAGE_BUFFER *build_message_queue(int size)
{
    MESSAGE_BUFFER *message_buffer;

    if (size > MESSAGE_QUEUE_MAX_SIZE) size = MESSAGE_QUEUE_MAX_SIZE;
    if (size < MESSAGE_QUEUE_MIN_SIZE) size = MESSAGE_QUEUE_MIN_SIZE;

    if ((message_buffer = (MESSAGE_BUFFER*)my_malloc(sizeof(MESSAGE_BUFFER))) == NULL)
        return NULL;

    message_buffer->size = size;
    message_buffer->wp = 0;
    message_buffer->rp = 0;
    message_buffer->count = 0;
    InitializeCriticalSection(&(message_buffer->SpinLock));

    if ((message_buffer->buffer = (char*)my_malloc(size)) == NULL)
        return NULL;

    return message_buffer;
}


void remove_message_queue(MESSAGE_BUFFER *message_buffer)
{
    if (message_buffer == NULL) return;
    DeleteCriticalSection(&(message_buffer->SpinLock));
    my_free(message_buffer->buffer);
    my_free(message_buffer);
}

int write_message_ts_as(int pid, int mid, uint64_t timestamp, int msize, const char *mblock, int tx_pid)
{
    int found = 0;
    int need_space;
    int free_space;
    int parta, partb;
    char *dest;
    char *src;
    MESSAGE_BUFFER *message_buffer;
    MESSAGE_HEAD mhead;

    if (msize < 0) return MESSAGE_SIZE_ERROR;

	if ((pid & RT_PID_BIT_MASK) != RT_PID_BIT_MASK) {
        // FiFo-ID will be used as traget PID
		return WriteToFiFo(pid, mid | MESSAGE_THROUGH_FIFO_MASK, tx_pid, timestamp, msize, mblock);
	}
    mhead.pid = tx_pid;
    mhead.mid = mid;
    mhead.timestamp = timestamp;
    mhead.size = msize;

    message_buffer = GetMessageQueueForProcess(pid);
    if (message_buffer == NULL) {
        return NO_MESSAGE_QUEUE;
    }
    /* check if there is enough space for the message */
    need_space = msize + sizeof(MESSAGE_HEAD);

    EnterCriticalSection(&(message_buffer->SpinLock));

    if (message_buffer->rp > message_buffer->wp) {
        free_space = message_buffer->rp - message_buffer->wp;
    }
    else {
        free_space = message_buffer->size - (message_buffer->wp - message_buffer->rp);
    }
    if (free_space <= need_space) {
        LeaveCriticalSection(&(message_buffer->SpinLock));
        return MESSAGE_OVERFLOW;
    }

    /* Write message */

    /* First the message head*/
    dest = (char*)&message_buffer->buffer[message_buffer->wp];
    src = (char*)&mhead;
    parta = message_buffer->size - message_buffer->wp;
    partb = sizeof(MESSAGE_HEAD) - parta;

    if (parta >= sizeof(MESSAGE_HEAD)) {
        MEMCPY(dest, src, sizeof(MESSAGE_HEAD));
        message_buffer->wp += sizeof(MESSAGE_HEAD);
    }
    else {
        MEMCPY(dest, src, parta);
        MEMCPY(message_buffer->buffer, &src[parta], partb);
        message_buffer->wp = partb;
    }

    /* Than the message data */
    dest = (char*)&message_buffer->buffer[message_buffer->wp];
    parta = message_buffer->size - message_buffer->wp;
    partb = msize - parta;

    if (parta >= msize) {
        MEMCPY(dest, mblock, msize);
        message_buffer->wp += msize;
    }
    else {
        MEMCPY(dest, mblock, parta);
        MEMCPY(message_buffer->buffer, &mblock[parta], partb);
        message_buffer->wp = partb;
    }
    message_buffer->count++;
    LeaveCriticalSection(&(message_buffer->SpinLock));
    return 0;
}

int write_message_ts(int pid, int mid, uint64_t timestamp, int msize, const char *mblock)
{
    return write_message_ts_as(pid, mid, timestamp, msize, mblock, GET_PID());
}

int write_message(int pid, int mid, int msize, const char *mblock)
{
    return (write_message_ts_as(pid, mid, get_timestamp_counter(), msize, mblock, GET_PID()));
}

int write_message_as(int pid, int mid, int msize, const char *mblock, int tx_pid)
{
    return (write_message_ts_as(pid, mid, get_timestamp_counter(), msize, mblock, tx_pid));
}

int write_important_message_ts_as(int pid, int mid, uint64_t timestamp, int msize, const char *mblock, int tx_pid)
{
    int found = 0;
    int need_space;
    int free_space;
    int parta, partb;
    int help_rp;
    char *dest;
    char *src;
    MESSAGE_BUFFER *message_buffer;
    MESSAGE_HEAD mhead;

    if (msize < 0) return MESSAGE_SIZE_ERROR;

	if ((pid & RT_PID_BIT_MASK) != RT_PID_BIT_MASK) {
        // FiFo-ID will be used as traget PID
        return WriteToFiFo(pid, mid | IMPORTANT_MESSAGE_THROUGH_FIFO_MASK, tx_pid, timestamp, msize, mblock);
	}

    mhead.pid = GET_PID();
    mhead.mid = mid;
    mhead.timestamp = timestamp;
    mhead.size = msize;

    message_buffer = GetMessageQueueForProcess(pid);
    if (message_buffer == NULL) {
        return MESSAGE_PID_ERROR;
    }
    /* check if there is enough space for the message */
    need_space = msize + sizeof(MESSAGE_HEAD);

    EnterCriticalSection(&(message_buffer->SpinLock));

    if (message_buffer->rp > message_buffer->wp) {
        free_space = message_buffer->rp - message_buffer->wp;
    }
    else {
        free_space = message_buffer->size - (message_buffer->wp - message_buffer->rp);
    }
    if (free_space <= need_space) {
        LeaveCriticalSection(&(message_buffer->SpinLock));
        return MESSAGE_OVERFLOW;
    }

    /* Write the message */

    /* First the message head */
    message_buffer->rp -= need_space;
    if (message_buffer->rp < 0)
        message_buffer->rp = message_buffer->size + message_buffer->rp;
    dest = (char*)&message_buffer->buffer[message_buffer->rp];
    src = (char*)&mhead;
    parta = message_buffer->size - message_buffer->rp;
    partb = sizeof(MESSAGE_HEAD) - parta;

    if (parta >= sizeof(MESSAGE_HEAD)) {
        MEMCPY(dest, src, sizeof(MESSAGE_HEAD));
        help_rp = message_buffer->rp + sizeof(MESSAGE_HEAD);
    }
    else {
        MEMCPY(dest, src, parta);
        MEMCPY(message_buffer->buffer, &src[parta], partb);
        help_rp = partb;
    }

    /* Than the message data*/
    dest = (char*)&message_buffer->buffer[help_rp];
    parta = message_buffer->size - help_rp;
    partb = msize - parta;

    if (parta >= msize) {
        MEMCPY(dest, mblock, msize);
    }
    else {
        MEMCPY(dest, mblock, parta);
        MEMCPY(message_buffer->buffer, &mblock[parta], partb);
    }
    message_buffer->count++;
    LeaveCriticalSection(&(message_buffer->SpinLock));
    return 0;
}

int write_important_message_ts(int pid, int mid, unsigned long timestamp, int msize, const char *mblock)
{
    return write_important_message_ts_as(pid, mid, timestamp, msize, mblock, GET_PID());
}

int write_important_message(int pid, int mid, int msize, const char *mblock)
{
    return (write_important_message_ts(pid, mid, get_timestamp_counter(), msize, mblock));
}


int test_message_as(MESSAGE_HEAD *mhead, int pid)
{
    char *dest;
    char *src;
    int parta, partb;
    MESSAGE_BUFFER *message_buffer;

    message_buffer = GetMessageQueueForProcess(pid);
    if ((message_buffer == NULL)  ||             // Have no message queue
        (message_buffer->count == 0)) {          // or message queue is empty
        return 0;
    }
    /* Copy message head */
    dest = (char*)mhead;
    EnterCriticalSection(&(message_buffer->SpinLock));
    src = (char*)&message_buffer->buffer[message_buffer->rp];

    parta = message_buffer->size - message_buffer->rp;
    partb = sizeof(MESSAGE_HEAD) - parta;

    if (parta >= sizeof(MESSAGE_HEAD)) {
        MEMCPY(dest, src, sizeof(MESSAGE_HEAD));
    }
    else {
        MEMCPY(dest, src, parta);
        MEMCPY(&dest[parta], message_buffer->buffer, partb);
    }
    LeaveCriticalSection(&(message_buffer->SpinLock));
    return 1;
}

int test_message(MESSAGE_HEAD *mhead)
{
    return test_message_as(mhead, GET_PID());
}


int read_message_as(MESSAGE_HEAD *mhead, char *message, int maxlen, int pid)
{
    char *dest;
    char *src;
    int parta, partb;
    MESSAGE_BUFFER *message_buffer;
    int message_count;

    message_buffer = GetMessageQueueForProcess(pid);
    if ((message_buffer == NULL) ||         // Have no message queue
        (message_buffer->count == 0)) {     // or message queue is empty
        return 0;
    }
    /* Copy message head */
    dest = (char*)mhead;
    EnterCriticalSection(&(message_buffer->SpinLock));
    src = (char*)&message_buffer->buffer[message_buffer->rp];

    parta = message_buffer->size - message_buffer->rp;
    partb = sizeof(MESSAGE_HEAD) - parta;

    if (parta >= sizeof(MESSAGE_HEAD)) {
        MEMCPY(dest, src, sizeof(MESSAGE_HEAD));
        message_buffer->rp += sizeof(MESSAGE_HEAD);
    }
    else {
        MEMCPY(dest, src, parta);
        MEMCPY(&dest[parta], message_buffer->buffer, partb);
        message_buffer->rp = partb;
    }

    if (maxlen < mhead->size) {
        LeaveCriticalSection(&(message_buffer->SpinLock));
        return MESSAGE_SIZE_ERROR;
    }

    /* Than the message data */
    src = (char*)&message_buffer->buffer[message_buffer->rp];
    parta = message_buffer->size - message_buffer->rp;
    partb = mhead->size - parta;

    if (parta >= mhead->size) {
        MEMCPY(message, src, mhead->size);
        message_buffer->rp += mhead->size;
    }
    else {
        MEMCPY(message, src, parta);
        MEMCPY(&message[parta], message_buffer->buffer, partb);
        message_buffer->rp = partb;
    }
    message_count = --(message_buffer->count);
    LeaveCriticalSection(&(message_buffer->SpinLock));
    return (message_count);
}

int read_message(MESSAGE_HEAD *mhead, char *message, int maxlen)
{
    return read_message_as(mhead, message, maxlen, GET_PID());
}


int remove_message_as(int pid)
{
    int parta, partb;
    char *dest;
    char *src;
    MESSAGE_BUFFER *message_buffer;
    MESSAGE_HEAD mhead;
    int message_count;

    message_buffer = GetMessageQueueForProcess(pid);
    if ((message_buffer == NULL) ||      // Have no message queue
        (message_buffer->count == 0)) {  // or message queue is empty
        return 0;
    }

    /* Copy message head */
    dest = (char*)&mhead;
    EnterCriticalSection(&(message_buffer->SpinLock));
    src = (char*)&message_buffer->buffer[message_buffer->rp];

    parta = message_buffer->size - message_buffer->rp;
    partb = sizeof(MESSAGE_HEAD) - parta;

    if (parta >= sizeof(MESSAGE_HEAD)) {
        MEMCPY(dest, src, sizeof(MESSAGE_HEAD));
        message_buffer->rp += sizeof(MESSAGE_HEAD);
    }
    else {
        MEMCPY(dest, src, parta);
        MEMCPY(&dest[parta], message_buffer->buffer, partb);
        message_buffer->rp = partb;
    }

    parta = message_buffer->size - message_buffer->rp;
    partb = mhead.size - parta;

    if (parta >= mhead.size) {
        message_buffer->rp += mhead.size;
    }
    else {
        message_buffer->rp = partb;
    }

    message_count = --(message_buffer->count);
    LeaveCriticalSection(&(message_buffer->SpinLock));
    return (message_count);
}

int remove_message(void)
{
    return remove_message_as(GET_PID());
}
