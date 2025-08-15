//LLM
/*
 * A generic, linked-list-based queue library in C.
 * To use, simply #include "queue.h" in your file.
 */
#ifndef QUEUE_H
#define QUEUE_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

// --- Data Structures ---

// The node structure for each element in the queue's linked list.
typedef struct Node {
    void* data;         // Generic pointer to hold any data type
    struct Node* next;  // Pointer to the next node in the queue
} Node;

// The queue structure.
typedef struct Queue {
    Node* front;        // Points to the front of the queue
    Node* rear;         // Points to the rear of the queue
    int size;           // The number of elements in the queue
} Queue;


// --- Function Prototypes ---

/**
 * @brief Creates and initializes a new, empty queue.
 * @return A pointer to the newly created Queue, or NULL if memory allocation fails.
 */
Queue* queue_create();

/**
 * @brief Adds an element to the rear of the queue (enqueue).
 * @param q A pointer to the Queue.
 * @param data A pointer to the data to be stored.
 */
void enqueue(Queue* q, void* data);

/**
 * @brief Removes and returns the element from the front of the queue (dequeue).
 * @param q A pointer to the Queue.
 * @return A pointer to the data from the front of the queue, or NULL if the queue is empty.
 */
void* dequeue(Queue* q);

/**
 * @brief Returns the element at the front of the queue without removing it.
 * @param q A pointer to the Queue.
 * @return A pointer to the data from the front of the queue, or NULL if the queue is empty.
 */
void* queue_peek(const Queue* q);

/**
 * @brief Gets the element at a specific index in the queue without removing it.
 * @param q A pointer to the Queue.
 * @param index The zero-based index of the element to retrieve.
 * @return A pointer to the data at the specified index, or NULL if the index is out of bounds.
 */
void* queue_get_at(const Queue* q, int index);

/**
 * @brief Gets the current number of elements in the queue.
 * @param q A pointer to the Queue.
 * @return The integer size of the queue.
 */
int queue_get_size(const Queue* q);

/**
 * @brief Checks if the queue is empty.
 * @param q A pointer to the Queue.
 * @return True if the queue is empty, false otherwise.
 */
bool queue_is_empty(const Queue* q);

/**
 * @brief Frees all memory used by the queue.
 * @param q A pointer to the Queue to be freed.
 * @param free_data A function pointer to a function that can free the data stored
 * in the queue. If NULL, the data itself is not freed.
 */
void queue_free(Queue* q, void (*free_data)(void*));

#endif // QUEUE_H
//LLM
