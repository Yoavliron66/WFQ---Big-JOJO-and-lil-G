// Include all used API's
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// Define the maximum line Length for reading input
#define MAX_LINE_LENGTH 512
// Define the maximum number of flows
#define MAX_FLOWS 2000
// Defone the maximum number of packets
#define MAX_PACKETS 100000
// Define the flow structure
typedef struct FLOW{
    int source_port;
    int destination_port;
    char source_ip[16]; // IPv4 address in dotted decimal format
    char destination_ip[16]; // IPv4 address in dotted decimal format
    int weight; // Weight of the flow
    int packet_count_active; // The number of active packets in the WFQ queue 
    float last_finished_time; // The last finished time of the flow
    struct FLOW* next; // Pointer to the next flow in the linked list
} flow;

// Define the packet structure
typedef struct PACKET{
    int source_port;
    int destination_port;
    char source_ip[16]; // IPv4 address in dotted decimal format
    char destination_ip[16]; // IPv4 address in dotted decimal format
    int weight; // Weight of the flow
    int length; // Length of the packet
    int arrival_time; // The arrival time of the packet
    float virtual_arrival_time; // The virtual arrival time of the packet
    float virtual_finish_time; // The virtual finish time of the packet
} packet;


/////////////////////
// MIN HEAP IMPLEMENTATION
/////////////////////
#define MAX_HEAP_SIZE MAX_PACKETS

typedef struct HEAP {
    packet* packets[MAX_HEAP_SIZE];
    int size;
} heap;

int compare_packets(const packet* a, const packet* b) {
    if (a->virtual_finish_time < b->virtual_finish_time) return -1;
    if (a->virtual_finish_time > b->virtual_finish_time) return 1;
    if (a->arrival_time < b->arrival_time) return -1;
    if (a->arrival_time > b->arrival_time) return 1;
    return 0;
}

void swap(packet** heap, int i, int j) {
    packet* temp = heap[i];
    heap[i] = heap[j];
    heap[j] = temp;
}

void heapify_up(packet** heap, int index) {
    while (index > 0) {
        int parent = (index - 1) / 2;
        if (compare_packets(heap[index], heap[parent]) < 0) {
            swap(heap, index, parent);
            index = parent;
        } else {
            break;
        }
    }
}

void heapify_down(packet** heap, int heap_size, int index) {
    while (1) {
        int smallest = index;
        int left = 2 * index + 1;
        int right = 2 * index + 2;

        if (left < heap_size && compare_packets(heap[left], heap[smallest]) < 0)
            smallest = left;
        if (right < heap_size && compare_packets(heap[right], heap[smallest]) < 0)
            smallest = right;

        if (smallest != index) {
            swap(heap, index, smallest);
            index = smallest;
        } else {
            break;
        }
    }
}

void push_packet(packet** heap, int* heap_size, packet* p) {
    if (*heap_size >= MAX_HEAP_SIZE) {
        fprintf(stderr, "Heap overflow\n");
        exit(EXIT_FAILURE);
    }
    heap[*heap_size] = p;
    heapify_up(heap, *heap_size);
    (*heap_size)++;
}

packet* pop_packet(packet** heap, int* heap_size) {
    if (*heap_size == 0) return NULL;
    packet* top = heap[0];
    (*heap_size)--;
    heap[0] = heap[*heap_size];
    heapify_down(heap, *heap_size, 0);
    return top;
}

packet* top_packet(packet** heap, int heap_size) {
    return heap_size > 0 ? heap[0] : NULL;
}

void init_heap(heap* h) {
    h->size = 0;
    for (int i = 0; i < MAX_HEAP_SIZE; i++) {
        h->packets[i] = NULL;
    }
}


////////////////////////////////////
// FLOW linked list functions
////////////////////////////////////

flow* create_flow(int source_port, int destination_port, const char* source_ip, const char* destination_ip, int weight) {
    flow* new_flow = (flow*)malloc(sizeof(flow));
    if (!new_flow) {
        fprintf(stderr, "Memory allocation failed for flow\n");
        exit(EXIT_FAILURE);
    }
    new_flow->source_port = source_port;
    new_flow->destination_port = destination_port;
    strncpy(new_flow->source_ip, source_ip, 15);
    new_flow->source_ip[15] = '\0'; // Ensure null termination
    strncpy(new_flow->destination_ip, destination_ip, 15);
    new_flow->destination_ip[15] = '\0'; // Ensure null termination
    new_flow->weight = weight;
    new_flow->packet_count_active = 1; // If a new flow is created, it has at least one active packet
    new_flow->last_finished_time = 0.0f;
    new_flow->next = NULL;
    return new_flow;
}


void append_flow(flow** head, flow* new_flow) {
    if (*head == NULL) {
        *head = new_flow;
    } else {
        flow* current = *head;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = new_flow;
    }
}

void remove_flow(flow** head, flow* target_flow) {
    if (*head == NULL) return;

    if (*head == target_flow) {
        flow* temp = *head;
        *head = (*head)->next;
        free(temp);
        return;
    }

    flow* current = *head;
    while (current->next != NULL && current->next != target_flow) {
        current = current->next;
    }

    if (current->next == target_flow) {
        flow* temp = current->next;
        current->next = current->next->next;
        free(temp);
    }
}

void free_flows(flow* head) {
    flow* current = head;
    while (current != NULL) {
        flow* next = current->next;
        free(current);
        current = next;
    }
}

int total_active_weights(flow* head) {
    int total_weight = 0;
    flow* current = head;
    while (current != NULL) {
        if (current->packet_count_active > 0) {
            total_weight += current->weight;
        }
        current = current->next;
    }
    return total_weight;
}


// max float function
float max_float(float a, float b) {
    return (a > b) ? a : b;
}