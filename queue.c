#include <stdio.h>
#include <stdlib.h>
#include "queue.h"

Queue* createQueue() {
  Queue* queue = (Queue*)malloc(sizeof(Queue));
  queue->front = queue->size = 0;
  queue->rear = QUEUE_SIZE - 1;
  queue->array = (char**)malloc(QUEUE_SIZE * sizeof(char*));
  return queue;
}

int isFull(Queue* queue) {
  return (queue->size == QUEUE_SIZE);
}

int isEmpty(Queue* queue) {
  return (queue->size == 0);
}

void enqueue(Queue* queue, char* item) {
  if (isFull(queue))
    return;
  queue->rear = (queue->rear + 1) % QUEUE_SIZE;
  queue->array[queue->rear] = item;
  queue->size = queue->size + 1;
}

// gotta make sure i run free() on the char* returned
char* dequeue(Queue* queue) {
  if (isEmpty(queue))
    return NULL;
  char* item = queue->array[queue->front];
  queue->front = (queue->front + 1) % QUEUE_SIZE;
  queue->size = queue->size - 1;
  return item;
}

//   Queue* queue = createQueue();

//   char* item1 = (char*)malloc(ARRAY_SIZE * sizeof(char));

//   char* dequeuedItem1 = dequeue(queue);
