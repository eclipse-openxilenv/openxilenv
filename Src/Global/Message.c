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


#define MESSAGE_C
#include <stdio.h>
//#include <mem.h>
#include "Config.h"
#include "Scheduler.h"
#include "MyMemory.h"
#include "StringMaxChar.h"
#include "Fifos.h"
#include "Message.h"

#ifndef REMOTE_MASTER
#include "MainValues.h"
#include "RemoteMasterMessage.h"
#endif

// Also inside Scheder.c !!!
#define RT_PID_BIT_MASK          0x10000000

static CRITICAL_SECTION MessageCriticalSection;
static int IsMessageCriticalSectionInit;

MESSAGE_BUFFER *build_message_queue (int size)
{
    MESSAGE_BUFFER *message_buffer;

    // This should be inside an own init function
    if (!IsMessageCriticalSectionInit) {
        InitializeCriticalSection (&MessageCriticalSection);
        IsMessageCriticalSectionInit = 1;
    }
    if (size > MESSAGE_QUEUE_MAX_SIZE) size = MESSAGE_QUEUE_MAX_SIZE;
    if (size < MESSAGE_QUEUE_MIN_SIZE) size = MESSAGE_QUEUE_MIN_SIZE;

    if ((message_buffer = (MESSAGE_BUFFER*)my_malloc (sizeof (MESSAGE_BUFFER))) == NULL)
        return NULL;

    message_buffer->size = size;
    message_buffer->wp = 0;
    message_buffer->rp = 0;
    message_buffer->count = 0;

    if ((message_buffer->buffer = (char*)my_malloc (size)) == NULL)
        return NULL;

    return message_buffer;
}


void remove_message_queue (MESSAGE_BUFFER *message_buffer)
{
    if (message_buffer == NULL) return; 
    my_free (message_buffer->buffer);
    my_free (message_buffer);
}

int write_message_ts_as (int pid, int mid, uint64_t timestamp, int msize, const char *mblock, int tx_pid)
{
    TASK_CONTROL_BLOCK *tcb;
    int need_space;
    int free_space;
    int parta, partb;
    char *dest;
    char *src;
    MESSAGE_BUFFER *message_buffer;
    MESSAGE_HEAD mhead;

#ifndef REMOTE_MASTER
    if (s_main_ini_val.ConnectToRemoteMaster &&
        ((pid & RT_PID_BIT_MASK) == RT_PID_BIT_MASK)) {
        // FiFo-ID would be used as target PID
        return WriteToFiFo (pid, (uint32_t)(mid | MESSAGE_THROUGH_FIFO_MASK), tx_pid, timestamp, msize, mblock);
    }
#endif

    if (msize < 0) return MESSAGE_SIZE_ERROR;

    mhead.pid = tx_pid;
    mhead.mid = mid;
    mhead.timestamp = timestamp;
    mhead.size = msize;

    tcb = GetPointerToTaskControlBlock (pid);
    if (tcb == NULL) {
        return MESSAGE_PID_ERROR;
    }
    if ((!tcb->message_size) || (tcb->message_buffer == NULL)) {
        return NO_MESSAGE_QUEUE;
    }
    /* Check if there is enough space for this message */
    need_space = msize + (int)sizeof (MESSAGE_HEAD);

    EnterCriticalSection (&MessageCriticalSection);

    message_buffer = tcb->message_buffer;
    if (message_buffer->rp > message_buffer->wp) {
        free_space = message_buffer->rp - message_buffer->wp;
    } else {
        free_space = message_buffer->size - (message_buffer->wp - message_buffer->rp);
    }
    if (free_space <= need_space) {
        LeaveCriticalSection (&MessageCriticalSection);
        return MESSAGE_OVERFLOW;
    }

    /* Write the message */

    /* First write the message head */
    dest = (char*)&message_buffer->buffer[message_buffer->wp];
    src = (char*)&mhead;
    parta = message_buffer->size - message_buffer->wp;
    partb = (int)sizeof (MESSAGE_HEAD) - parta;

    if (parta >= (int)sizeof (MESSAGE_HEAD)) {
        MEMCPY (dest, src, sizeof (MESSAGE_HEAD));
        message_buffer->wp += sizeof (MESSAGE_HEAD);
    } else {
        MEMCPY (dest, src, (size_t)parta);
        MEMCPY (message_buffer->buffer, &src[parta], (size_t)partb);
        message_buffer->wp = partb;
    }

    /* Than the data */
    dest = (char*)&message_buffer->buffer[message_buffer->wp];
    parta = message_buffer->size - message_buffer->wp;
    partb = msize - parta;

    if (parta >= msize) {
        MEMCPY (dest, mblock, (size_t)msize);
        message_buffer->wp += msize;
    } else {
        MEMCPY (dest, mblock, (size_t)parta);
        MEMCPY (message_buffer->buffer, &mblock[parta], (size_t)partb);
        message_buffer->wp = partb;
    }
    message_buffer->count++;
    LeaveCriticalSection (&MessageCriticalSection);
    return 0;
}

int write_message_ts (int pid, int mid, uint64_t timestamp, int msize, const char *mblock)
{
    return write_message_ts_as (pid, mid, timestamp, msize, mblock, GET_PID ());
}

int write_message (int pid, int mid, int msize, const char *mblock)
{
    return (write_message_ts_as (pid, mid, get_timestamp_counter(),  msize, mblock, GET_PID ()));
}

int write_message_as (int pid, int mid, int msize, const char *mblock, int tx_pid)
{
    return (write_message_ts_as (pid, mid, get_timestamp_counter(),  msize, mblock, tx_pid));
}

int write_important_message_ts_as (int pid, int mid, uint64_t timestamp, int msize, const char *mblock, int tx_pid)
{
    TASK_CONTROL_BLOCK *tcb;
    int need_space;
    int free_space;
    int parta, partb;
    int help_rp;
    char *dest;
    char *src;
    MESSAGE_BUFFER *message_buffer;
    MESSAGE_HEAD mhead;

#ifndef REMOTE_MASTER
    if (s_main_ini_val.ConnectToRemoteMaster &&
        ((pid & RT_PID_BIT_MASK) == RT_PID_BIT_MASK)) {
        // FiFo-ID would be used as target PID
        return WriteToFiFo (pid, (uint32_t)(mid | IMPORTANT_MESSAGE_THROUGH_FIFO_MASK), tx_pid, timestamp, msize, mblock);
    }
#endif

    if (msize < 0) return MESSAGE_SIZE_ERROR;

    mhead.pid = tx_pid;
    mhead.mid = mid;
    mhead.timestamp = timestamp;
    mhead.size = msize;

    /* Check if process (pid) is valid and he own a message queue */
    tcb = GetPointerToTaskControlBlock (pid);
    if (tcb == NULL) {
        return MESSAGE_PID_ERROR;
    }
    if ((!tcb->message_size) || (tcb->message_buffer == NULL)) {
        return NO_MESSAGE_QUEUE;
    }
    /* Check if there is enough space for this message */
    need_space = msize + (int)sizeof (MESSAGE_HEAD);

    EnterCriticalSection (&MessageCriticalSection);

    message_buffer = tcb->message_buffer;
    if (message_buffer->rp > message_buffer->wp) {
        free_space = message_buffer->rp - message_buffer->wp;
    } else {
        free_space = message_buffer->size - (message_buffer->wp - message_buffer->rp);
    }
    if (free_space <= need_space) {
        LeaveCriticalSection (&MessageCriticalSection);
        return MESSAGE_OVERFLOW;
    }

    /* Write the message */

    /* First write the message head */
    message_buffer->rp -= need_space;
    if (message_buffer->rp < 0)
        message_buffer->rp = message_buffer->size + message_buffer->rp;
    dest = (char*)&message_buffer->buffer[message_buffer->rp];
    src = (char*)&mhead;
    parta = message_buffer->size - message_buffer->rp;
    partb = (int)sizeof (MESSAGE_HEAD) - parta;

    if (parta >= (int)sizeof (MESSAGE_HEAD)) {
        MEMCPY (dest, src, sizeof (MESSAGE_HEAD));
        help_rp = message_buffer->rp + (int)sizeof (MESSAGE_HEAD);
    } else {
        MEMCPY (dest, src, (size_t)parta);
        MEMCPY (message_buffer->buffer, &src[parta], (size_t)partb);
        help_rp = partb;
    }

    /* Than the data */
    dest = (char*)&message_buffer->buffer[help_rp];
    parta = message_buffer->size - help_rp;
    partb = msize - parta;

    if (parta >= msize) {
        MEMCPY (dest, mblock, (size_t)msize);
    } else {
        MEMCPY (dest, mblock, (size_t)parta);
        MEMCPY (message_buffer->buffer, &mblock[parta], (size_t)partb);
    }
    message_buffer->count++;
    LeaveCriticalSection (&MessageCriticalSection);
    return 0;
}

int write_important_message (int pid, int mid, int msize, const char *mblock)
{
    return (write_important_message_ts_as (pid, mid, get_timestamp_counter(),  msize, mblock, GET_PID ()));
}


int test_message (MESSAGE_HEAD *mhead)
{
    TASK_CONTROL_BLOCK *tcb;
    char *dest;
    char *src;
    int parta, partb;
    MESSAGE_BUFFER *message_buffer;

    tcb = GET_TCB ();
    if ((!tcb->message_size) ||                  /* Process has no message queue */
        (tcb->message_buffer->count == 0)) {     /* or message queue is empty */
        return 0;
    }
    /* Copy message head */
    message_buffer = tcb->message_buffer;
    dest = (char*)mhead;
    EnterCriticalSection (&MessageCriticalSection);
    src = (char*)&message_buffer->buffer[message_buffer->rp];

    parta = message_buffer->size - message_buffer->rp;
    partb = (int)sizeof (MESSAGE_HEAD) - parta;

    if (parta >= (int)sizeof (MESSAGE_HEAD)) {
        MEMCPY (dest, src, sizeof (MESSAGE_HEAD));
    } else {
        MEMCPY (dest, src, (size_t)parta);
        MEMCPY (&dest[parta], message_buffer->buffer, (size_t)partb);
    }
    /* printf ("message head: pid = %i, mid = %i, size = %i, ts = %i\n",
               mhead->pid, mhead->mid, mhead->size, mhead->timestamp); */
    LeaveCriticalSection (&MessageCriticalSection);
    return 1;
}

int read_message (MESSAGE_HEAD *mhead, char *message, int maxlen)
{
    TASK_CONTROL_BLOCK *tcb;
    char *dest;
    char *src;
    int parta, partb;
    MESSAGE_BUFFER *message_buffer;
    int message_count;

    tcb = GET_TCB ();
    if ((!tcb->message_size) ||                 /* Process has no message queue  */
        (tcb->message_buffer->count == 0)) {    /* or message queue is empty */
        return 0;
    }
    /* Copy message head */
    message_buffer = tcb->message_buffer;
    dest = (char*)mhead;
    EnterCriticalSection (&MessageCriticalSection);
    src = (char*)&message_buffer->buffer[message_buffer->rp];

    parta = message_buffer->size - message_buffer->rp;
    partb = (int)sizeof (MESSAGE_HEAD) - parta;

    if (parta >= (int)sizeof (MESSAGE_HEAD)) {
        MEMCPY (dest, src, sizeof (MESSAGE_HEAD));
        message_buffer->rp += sizeof (MESSAGE_HEAD);
    } else {
        MEMCPY (dest, src, (size_t)parta);
        MEMCPY (&dest[parta], message_buffer->buffer, (size_t)partb);
        message_buffer->rp = partb;
    }
    /* printf ("message head: pid = %i, mid = %i, size = %i, ts = %i\n",
               mhead->pid, mhead->mid, mhead->size, mhead->timestamp); */

    if (maxlen < mhead->size) {
        LeaveCriticalSection (&MessageCriticalSection);
        return MESSAGE_SIZE_ERROR;
    }

    /* Copy data */
    src = (char*)&message_buffer->buffer[message_buffer->rp];
    parta = message_buffer->size - message_buffer->rp;
    partb = mhead->size - parta;

    if (parta >= mhead->size) {
        MEMCPY (message, src, (size_t)mhead->size);
        message_buffer->rp += mhead->size;
    } else {
        MEMCPY (message, src, (size_t)parta);
        MEMCPY (&message[parta], message_buffer->buffer, (size_t)partb);
        message_buffer->rp = partb;
    }
    message_count = --(message_buffer->count);
    LeaveCriticalSection (&MessageCriticalSection);
    return (message_count);
}

int remove_message (void)
{
    TASK_CONTROL_BLOCK *tcb;
    int parta, partb;
    char *dest;
    char *src;
    MESSAGE_BUFFER *message_buffer;
    MESSAGE_HEAD mhead;
    int message_count;

    tcb = GET_TCB ();
    if ((!tcb->message_size) ||                 /* Process has no message queue */
        (tcb->message_buffer->count == 0)) {    /* or message queue is empty */
        return 0;
    }

    /* Copy message head */
    message_buffer = tcb->message_buffer;
    dest = (char*)&mhead;
    EnterCriticalSection (&MessageCriticalSection);
    src = (char*)&message_buffer->buffer[message_buffer->rp];

    parta = message_buffer->size - message_buffer->rp;
    partb = (int)sizeof (MESSAGE_HEAD) - parta;

    if (parta >= (int)sizeof (MESSAGE_HEAD)) {
        MEMCPY (dest, src, sizeof (MESSAGE_HEAD));
        message_buffer->rp += sizeof (MESSAGE_HEAD);
    } else {
        MEMCPY (dest, src, (size_t)parta);
        MEMCPY (&dest[parta], message_buffer->buffer, (size_t)partb);
        message_buffer->rp = partb;
    }

    parta = message_buffer->size - message_buffer->rp;
    partb = mhead.size - parta;

    if (parta >= mhead.size) {
        message_buffer->rp += mhead.size;
    } else {
        message_buffer->rp = partb;
    }

    message_count = --(message_buffer->count);
    LeaveCriticalSection (&MessageCriticalSection);
    return (message_count);
}

