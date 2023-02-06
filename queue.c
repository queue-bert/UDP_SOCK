#include <stdio.h>
#include <stdlib.h>

#define QUEUE_SIZE 100
#define ARRAY_SIZE 100

typedef struct {
  int front, rear;
  int size;
  char** array;
} Queue;

Queue* createQueue() {
  Queue* queue = (Queue*)malloc(sizeof(Queue));
  queue->front = queue->size = 0;
  queue->rear = QUEUE_SIZE - 1;
  queue->array = (char**)malloc(QUEUE_SIZE * sizeof(char*));
  return queue;
}

int isFull(Queue* queue) {
  return ((queue->rear + 1) % QUEUE_SIZE == queue->front);
}

int isEmpty(Queue* queue) {
  return (queue->front == queue->rear);
}

void enqueue(Queue* queue, char* item) {
  if (isFull(queue))
    return;
  queue->rear = (queue->rear + 1) % QUEUE_SIZE;
  queue->array[queue->rear] = item;
  queue->size = queue->size + 1;
}

char* dequeue(Queue* queue) {
  if (isEmpty(queue))
    return NULL;
  char* item = queue->array[queue->front];
  queue->front = (queue->front + 1) % QUEUE_SIZE;
  queue->size = queue->size - 1;
  return item;
}

int main() {
  Queue* queue = createQueue();

  char* item1 = (char*)malloc(ARRAY_SIZE * sizeof(char));
  char* item2 = (char*)malloc(ARRAY_SIZE * sizeof(char));
  char* item3 = (char*)malloc(ARRAY_SIZE * sizeof(char));

  enqueue(queue, item1);
  enqueue(queue, item2);
  enqueue(queue, item3);

  char* dequeuedItem1 = dequeue(queue);
  char* dequeuedItem2 = dequeue(queue);
  char* dequeuedItem3 = dequeue(queue);

  printf("Dequeued item 1: %s\n", dequeuedItem1);
  printf("Dequeued item 2: %s\n", dequeuedItem2);
  printf("Dequeued item 3: %s\n", dequeuedItem3);

  return 0;
}
