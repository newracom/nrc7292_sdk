#ifndef util_tiny_queue_h
#define util_tiny_queue_h

struct Element {
  /* temp fix for data type*/
  int data;
  struct Element *next;
  short idx;
};

struct Queue {
  int qsize;  
  struct Element *head;
  struct Element *tail;
};

void util_tque_init_queue(struct Queue *q);
int util_tque_enqueue(struct Queue *q, int data);
void util_tque_dequeue(struct Queue *q, int *data);
int util_tque_size(struct Queue *q);
#endif