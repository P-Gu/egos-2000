/*
 * (C) 2023, Cornell University
 * All rights reserved.
 */

/* Author: Robbert van Renesse
 * Description: course project, user-level threading
 * Students implement a threading package and semaphore;
 * And then spawn multiple threads as either producer or consumer.
 */

#include "app.h"
#include <stdlib.h>

// Structure to represent a node in the linked list
typedef struct Node {
    void *data;
    struct Node* next;
} Node;

// Structure to represent a queue
typedef struct Queue {
    Node* front;
    Node* rear;
} Queue;

// Function to create a new queue
Queue* createQueue() {
    Queue* queue = (Queue*)malloc(sizeof(Queue));
    if (queue != NULL) {
        queue->front = NULL;
        queue->rear = NULL;
    }
    return queue;
}

// Function to check if the queue is empty
int isEmpty(Queue* queue) {
    return queue->front == NULL;
}

// Function to enqueue (push) an element into the queue
void enqueue(Queue* queue, void *value) {
    Node* newNode = (Node*)malloc(sizeof(Node));
    if (newNode != NULL) {
        newNode->data = value;
        newNode->next = NULL;
        if (isEmpty(queue)) {
            queue->front = newNode;
            queue->rear = newNode;
        } else {
            queue->rear->next = newNode;
            queue->rear = newNode;
        }
    }
}

// Function to dequeue (pop) an element from the queue
void *dequeue(Queue* queue) {
    if (!isEmpty(queue)) {
        Node* temp = queue->front;
        void *value = temp->data;
        queue->front = temp->next;
        free(temp);
        return value;
    }
    return NULL;
}

// Function to free the memory allocated for the queue
void destroyQueue(Queue* queue) {
    while (!isEmpty(queue)) {
        free(dequeue(queue));
    }
    free(queue);
}

/** These two functions are defined in grass/context.S **/
void ctx_start(void** old_sp, void* new_sp);
void ctx_switch(void** old_sp, void* new_sp);

/** Multi-threading functions **/
struct thread *main_thread;
struct thread *current_thread;
int running_main;
struct Queue *q;
void *dummy;
int index = 0;

struct thread {
    /* Student's code goes here. */
    int id;
    void *sp;
    void *alloc_addr;
    int valid;
};

void thread_init(){
    /* Student's code goes here */
    INFO("init begin");
    main_thread = (struct thread *)malloc(sizeof(struct thread));
    main_thread->id = index++;
    main_thread->valid = 1;
    main_thread->alloc_addr = -1;
    q = createQueue();
    //enqueue(q, main_thread);
    current_thread = main_thread;
    INFO("init done %d", main_thread->id);
}

void ctx_entry(void){
    /* Student's code goes here. */
    INFO("ctx entry start %p\n", main_thread->sp);
    unsigned long f, arg;
    asm("lw %0, 0(sp)" : "=r"(arg));
    asm("lw %0, 16(sp)" : "=r"(f));

    asm("mv a0, %0" :: "r"(arg));
    asm("jalr t1, %0, 0" :: "r"(f));
    INFO("ctx entry done\n");
}

void thread_create(void (*f)(void *), void *arg, unsigned int stack_size){
    /* Student's code goes here. */
    INFO("create thread start\n"); 
    int *new_sp = malloc(stack_size+4);
    *(new_sp + stack_size - 20) = (void *)f;
    *(new_sp + stack_size - 24) = (void *)arg;
    struct thread *t = (struct thread *)malloc(sizeof(struct thread));
    enqueue(q, main_thread);
    current_thread = t;
    t->sp = new_sp + stack_size - 16;
    t->alloc_addr = new_sp;
    t->id = index++;
    t->valid = 1;
    ctx_start(&main_thread->sp, t->sp);

}      

void thread_yield(){
    /* Student's code goes here. */
    if (isEmpty(q)) {
        INFO("empty queue, yield ignored\n");
        return;
    }
    struct thread *prev_thread = current_thread;
    struct thread *next_thread = NULL;
    while (next_thread==NULL && (!isEmpty(q))) {
        next_thread = dequeue(q);
        INFO("yield dequeue %d", next_thread->id);
    }
    if (next_thread == NULL) {
        INFO("no valid thread, yield ignored\n");
        return;
    }
    enqueue(q, prev_thread);
    current_thread = next_thread;
    ctx_switch(&prev_thread->sp, next_thread->sp);
    INFO("yield thread done\n");
}

void thread_exit(){
    /* Student's code goes here. */
    INFO("thread exit start");
    if (isEmpty(q)) {
        INFO("empty queue, exit now\n");
        return;
    }
    struct thread *next_thread = NULL;
    while (next_thread==NULL && (!isEmpty(q))) {
        next_thread = dequeue(q);
        INFO("exit dequeue %d", next_thread->id);
    }
    if (next_thread == NULL) {
        INFO("no valid thread, yield ignored\n");
        return;
    }
    current_thread->valid = 0; 
    //free(current_thread->sp);
    if (current_thread->alloc_addr != -1) {
        free(current_thread->alloc_addr);
    }
    free(current_thread);
    current_thread = next_thread;
    ctx_switch(&dummy, next_thread->sp);
    INFO("thread exit done");
}

/** Semaphore functions **/

struct sema {
    /* Student's code goes here. */
};

void sema_init(struct sema *sema, unsigned int count){
    /* Student's code goes here. */
}

void sema_inc(struct sema *sema){
    /* Student's code goes here. */
}

void sema_dec(struct sema *sema){
    /* Student's code goes here. */
}

int sema_release(struct sema *sema){
    /* Student's code goes here. */
}

/** Producer and consumer functions **/

#define NSLOTS	3

static char *slots[NSLOTS];
static unsigned int in, out;
static struct sema s_empty, s_full;

static void producer(void *arg){
    for (;;) {
        // first make sure there's an empty slot.
        // then add an entry to the queue
        // lastly, signal consumers

        sema_dec(&s_empty);
        slots[in++] = arg;
        if (in == NSLOTS) in = 0;
        sema_inc(&s_full);
    }
}

static void consumer(void *arg){
    for (int i = 0; i < 5; i++) {
        // first make sure there's something in the buffer
        // then grab an entry to the queue
        // lastly, signal producers

        sema_dec(&s_full);
        void *x = slots[out++];
        if (out == NSLOTS) out = 0;
        printf("%s: got '%s'\n", arg, x);
        sema_inc(&s_empty);
    }
}

void test_code(void *arg) {
    for (int i = 0; i < 3; i++) {
        printf("%s here: %d\n", arg, i);
        thread_yield(); // switch the context back to main thread
    }
    printf("%s done\n", arg);
    thread_exit();
}

int main() {
    INFO("User-level threading is not implemented.");
    thread_init();
    thread_create(test_code, "thread 1", 16 * 1024);
    thread_create(test_code, "thread 2", 16 * 1024);
    test_code("main thread");
    thread_exit();

    /*
    thread_init();
    sema_init(&s_full, 0);
    sema_init(&s_empty, NSLOTS);

    thread_create(consumer, "consumer 1", 16 * 1024);
    thread_create(consumer, "consumer 2", 16 * 1024);
    thread_create(consumer, "consumer 3", 16 * 1024);
    thread_create(consumer, "consumer 4", 16 * 1024);
    thread_create(producer, "producer 2", 16 * 1024);
    thread_create(producer, "producer 3", 16 * 1024);
    producer("producer 1");
    thread_exit();
    */
    INFO("main function returns 0");
    return 0;
}

