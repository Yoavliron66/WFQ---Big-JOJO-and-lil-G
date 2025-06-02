#include "wfq.h"

int main(){
    // Create a heap
    heap h;
    init_heap(&h);
    // Create the flow linked list
    flow* flows = NULL;
    // The transimission variables
    int current_time = 0; // The current time in the simulation
    int previous_time = 0; // The previous time in the simulation
    int wfq_transmission_finish_time = 0; // The finish time of the WFQ transmission
    float virtual_time = 0.0; // The virtual time of the WFQ transmission
    
    ////////////////////////////////////
    /// Main simulation loop 
    /// Read the standard input
    ////////////////////////////////////
    char line[MAX_LINE_LENGTH];
    while(fgets(line, MAX_LINE_LENGTH, stdin) != NULL){
        // Parse the input line
        int source_port, destination_port, weight, length, arrival_time;
        char source_ip[16], destination_ip[16];
        int n;
        n = sscanf(line, "%d %15s %d %15s %d %d %d", &arrival_time, source_ip, &source_port, destination_ip, &destination_port, &length, &weight);
        if (n < 6) {
            fprintf(stderr, "Invalid input format: %s", line);
            exit(EXIT_FAILURE);
        }
        if (n == 6) {
            weight = 1; // Default weight if not provided
        } // end of parsing the input line
        // If the parsed time is greater then the WFQ transmission finish time, then we need to go through the new transmit process
        if (arrival_time > wfq_transmission_finish_time) {
            while (wfq_transmission_finish_time < arrival_time){
                // Update the current time
                previous_time = current_time;
                current_time = wfq_transmission_finish_time;
                // If the heap is not empty, we need to transmit the next packet
                if (h.size == 0) {
                    break; // No packets to transmit
                }
                // Get the next packet to transmit
                packet* next_packet = pop_packet(h.packets, &h.size);
                // Update the virtual time
                virtual_time = virtual_time + (float)(current_time - previous_time) / (float)total_active_weights(flows);
                // Print the start of the transmission
                fprintf(stdout, "%d: %d %15s %d %15s %d %d %d\n", current_time, next_packet->arrival_time, next_packet->source_ip, next_packet->source_port, next_packet->destination_ip, next_packet->destination_port, next_packet->length, next_packet->weight);
                // advance the wfq transmission finish time
                wfq_transmission_finish_time = current_time + next_packet->length;
                // Update the flow active packet count
                flow* current_flow = flows;
                while (current_flow != NULL) {
                    if (current_flow->source_port == next_packet->source_port && current_flow->destination_port == next_packet->destination_port &&
                        strcmp(current_flow->source_ip, next_packet->source_ip) == 0 && strcmp(current_flow->destination_ip, next_packet->destination_ip) == 0) {
                        current_flow->packet_count_active--;
                        if (current_flow->packet_count_active == 0) {
                            remove_flow(&flows, current_flow);
                        }
                        break; // Flow found
                    }
                    current_flow = current_flow->next;
                }
                // Free the packet memory
                free(next_packet);
            }    
        }
        // After this we are sure that the wfq transmission finish time is greater than the arrival time
        previous_time = current_time; // Update the previous time to the current time
        current_time = arrival_time; // Update the current time to the arrival time
        // If the heap is full, we cannot add more packets
        if (h.size >= MAX_HEAP_SIZE) {
            fprintf(stderr, "Heap overflow: cannot add more packets\n");
            exit(EXIT_FAILURE);
        }
        // Check if the flow already exists
        flow* current_flow = flows;
        while (current_flow != NULL) {
            if (current_flow->source_port == source_port && current_flow->destination_port == destination_port &&
                strcmp(current_flow->source_ip, source_ip) == 0 && strcmp(current_flow->destination_ip, destination_ip) == 0) {
                current_flow->packet_count_active++;    
                break; // Flow found
            }
            current_flow = current_flow->next;
        }
        // If the flow does not exist, create a new flow
        if (current_flow == NULL) {
            current_flow = create_flow(source_port, destination_port, source_ip, destination_ip, weight);
            append_flow(&flows, current_flow);
        }
        // Create a new packet
        packet* new_packet;
        new_packet = (packet*)malloc(sizeof(packet));
        if (!new_packet) {
            fprintf(stderr, "Memory allocation failed for packet\n");
            exit(EXIT_FAILURE);
        }
        // Initialize the new packet
        new_packet->source_port = source_port;
        new_packet->destination_port = destination_port;
        strncpy(new_packet->source_ip, source_ip, 15);
        new_packet->source_ip[15] = '\0'; // Ensure null termination
        strncpy(new_packet->destination_ip, destination_ip, 15);
        new_packet->destination_ip[15] = '\0'; // Ensure null termination
        new_packet->weight = weight;
        new_packet->length = length;
        new_packet->arrival_time = arrival_time;
        virtual_time = virtual_time + (float)(arrival_time - previous_time) / (float)total_active_weights(flows);
        new_packet->virtual_arrival_time = max_float(virtual_time, current_flow->last_finished_time);
        new_packet->virtual_finish_time = new_packet->virtual_arrival_time + (float)length / (float)current_flow->weight;
        // Update the flow last finished time
        current_flow->last_finished_time = new_packet->virtual_finish_time;
        // Add the new packet to the heap
        push_packet(h.packets, &h.size, new_packet);
    }   
    // After the input is finished, we need to transmit the remaining packets in the heap
    while (h.size > 0) {
        // Update the current time
        previous_time = current_time;   
        current_time = wfq_transmission_finish_time;
        // Get the next packet to transmit
        packet* next_packet = pop_packet(h.packets, &h.size);
        // Update the virtual time
        virtual_time = virtual_time + (float)(current_time - previous_time) / (float)total_active_weights(flows);
        // Print the start of the transmission
        fprintf(stdout, "%d: %d %15s %d %15s %d %d %d\n", current_time, next_packet->arrival_time, next_packet->source_ip, next_packet->source_port, next_packet->destination_ip, next_packet->destination_port, next_packet->length, next_packet->weight);
        // advance the wfq transmission finish time
        wfq_transmission_finish_time = current_time + next_packet->length;
        // Update the flow active packet count
        flow* current_flow = flows;
        while (current_flow != NULL) {
            if (current_flow->source_port == next_packet->source_port && current_flow->destination_port == next_packet->destination_port &&
                strcmp(current_flow->source_ip, next_packet->source_ip) == 0 && strcmp(current_flow->destination_ip, next_packet->destination_ip) == 0) {
                current_flow->packet_count_active--;
                if (current_flow->packet_count_active == 0) {
                    remove_flow(&flows, current_flow);
                }
                break; // Flow found
            }
            current_flow = current_flow->next;
        }
        // Free the packet memory
        free(next_packet);
    }    
}

