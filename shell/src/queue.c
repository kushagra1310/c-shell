#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/queue.h"

Queue* queue_create() {
    // Allocate memory for the queue structure
    Queue* q = (Queue*)malloc(sizeof(Queue));
    if (q == NULL) {
        perror("Failed to allocate memory for queue");
        return NULL;
    }

    // Initialize the queue's properties
    q->front = NULL;
    q->rear = NULL;
    q->size = 0;
    return q;
}

void enqueue(Queue* q, void* data) {
    if (q == NULL) return;

    // Create a new node
    Node* new_node = (Node*)malloc(sizeof(Node));
    if (new_node == NULL) {
        perror("Failed to allocate memory for new node");
        return;
    }
    new_node->data = data;
    new_node->next = NULL;

    // If the queue is empty, the new node is both front and rear
    if (q->rear == NULL) {
        q->front = new_node;
        q->rear = new_node;
    } else {
        // Otherwise, add the new node to the end and update the rear pointer
        q->rear->next = new_node;
        q->rear = new_node;
    }
    q->size++;
}

void* dequeue(Queue* q) {
    if (q == NULL || q->front == NULL) {
        return NULL; // Queue is empty
    }

    // Get the node at the front
    Node* temp_node = q->front;
    void* data = temp_node->data;

    // Move the front pointer to the next node
    q->front = q->front->next;

    // If the queue becomes empty, also update the rear pointer
    if (q->front == NULL) {
        q->rear = NULL;
    }

    // Free the old front node and decrement the size
    free(temp_node);
    q->size--;

    return data;
}

void* queue_peek(const Queue* q) {
    if (q == NULL || q->front == NULL) {
        return NULL; // Queue is empty
    }
    return q->front->data;
}

void* queue_get_at(const Queue* q, int index) {
    // Check for invalid queue or out-of-bounds index
    if (q == NULL || index < 0 || index >= q->size) {
        return NULL;
    }

    // Traverse the list from the front
    Node* current = q->front;
    for (int i = 0; i < index; i++) {
        current = current->next;
    }

    return current->data;
}

int queue_get_size(const Queue* q) {
    if (q == NULL) return 0;
    return q->size;
}

bool queue_is_empty(const Queue* q) {
    if (q == NULL) return true;
    return q->size == 0;
}

void queue_free(Queue* q, void (*free_data)(void*)) {
    if (q == NULL) return;

    Node* current = q->front;
    while (current != NULL) {
        Node* next = current->next;
        // If a data-freeing function is provided, use it
        if (free_data != NULL) {
            free_data(current->data);
        }
        free(current);
        current = next;
    }
    // Finally, free the queue structure itself
    free(q);
}