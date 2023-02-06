#ifndef QUEUE_H
#define QUEUE_H

#define QUEUE_SIZE 100

typedef struct {
  int front, rear;
  int size;
  char** array;
} Queue;

Queue* createQueue();

int isFull(Queue* queue);

int isEmpty(Queue* queue);

void enqueue(Queue* queue, char* item);

char* dequeue(Queue* queue);

#endif
