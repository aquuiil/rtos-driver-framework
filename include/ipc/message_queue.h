#ifndef MESSAGE_QUEUE_H
#define MESSAGE_QUEUE_H

#include "rtos_types.h"

typedef struct message_queue message_queue_t;

message_queue_t* mq_create(size_t msg_size, size_t max_msgs);
rtos_status_t mq_send(message_queue_t *mq, const void *msg, tick_t timeout);
rtos_status_t mq_receive(message_queue_t *mq, void *msg, tick_t timeout);
void mq_destroy(message_queue_t *mq);

#endif