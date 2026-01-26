#include "ipc/message_queue.h"
#include "kernel/memory.h"
#include <string.h>
#include <stdio.h>

struct message_queue {
    uint8_t *buffer;
    size_t msg_size;
    size_t max_msgs;
    size_t count;
    size_t head;
    size_t tail;
};

message_queue_t* mq_create(size_t msg_size, size_t max_msgs) {
    message_queue_t *mq = rtos_malloc(sizeof(message_queue_t));
    if (!mq) return NULL;
    
    mq->buffer = rtos_malloc(msg_size * max_msgs);
    if (!mq->buffer) {
        rtos_free(mq);
        return NULL;
    }
    
    mq->msg_size = msg_size;
    mq->max_msgs = max_msgs;
    mq->count = 0;
    mq->head = 0;
    mq->tail = 0;
    
    printf("[MQ] Created: %zu msgs × %zu bytes\n", max_msgs, msg_size);
    return mq;
}

rtos_status_t mq_send(message_queue_t *mq, const void *msg, tick_t timeout) {
    (void)timeout;
    if (!mq || !msg) return RTOS_ERROR_INVALID_PARAM;
    if (mq->count >= mq->max_msgs) return RTOS_ERROR_BUSY;
    
    memcpy(mq->buffer + (mq->head * mq->msg_size), msg, mq->msg_size);
    mq->head = (mq->head + 1) % mq->max_msgs;
    mq->count++;
    
    printf("[MQ] Sent (count=%zu)\n", mq->count);
    return RTOS_OK;
}

rtos_status_t mq_receive(message_queue_t *mq, void *msg, tick_t timeout) {
    (void)timeout;
    if (!mq || !msg) return RTOS_ERROR_INVALID_PARAM;
    if (mq->count == 0) return RTOS_ERROR_TIMEOUT;
    
    memcpy(msg, mq->buffer + (mq->tail * mq->msg_size), mq->msg_size);
    mq->tail = (mq->tail + 1) % mq->max_msgs;
    mq->count--;
    
    printf("[MQ] Received (count=%zu)\n", mq->count);
    return RTOS_OK;
}

void mq_destroy(message_queue_t *mq) {
    if (mq) {
        rtos_free(mq->buffer);
        rtos_free(mq);
        printf("[MQ] Destroyed\n");
    }
}