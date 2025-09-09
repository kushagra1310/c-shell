#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <error.h>
#include <errno.h>
#include "sham.h"
char *server_ip_address; // loopback address
int server_port;
uint32_t initial_seq_num = 1; // initial seq number
uint32_t current_seq_num = 1; // current seq number
char *input_file = NULL;
char *output_file = NULL;
int sliding_size = 10;
int ack_num = 0;
int window_size = WINDOW_SIZE * MAXPAYLOADSIZE;
FILE *log_file = NULL;
int logging_enabled = 0;
int available_buffer_size = 8092; // just for chat purposes to maintain uniformity
float loss_rate = 0.0;

// ############## LLM Generated Code Begins ##############
int sham_end_recieve(int socket, sham_packet incoming_packet, struct sockaddr_in *addr)
{
    // LLM
    incoming_packet.header.seq_num = ntohl(incoming_packet.header.seq_num);
    incoming_packet.header.flags = ntohs(incoming_packet.header.flags);

    char log_msg[100];
    // Step 1: Receive FIN
    sprintf(log_msg, "RCV FIN SEQ=%u", incoming_packet.header.seq_num);
    log_event(log_msg, log_file);

    printf("Other side initiated disconnect. Beginning 4-way termination...\n");

    // Step 2: Send ACK for their FIN
    sham_header ack_response;
    memset(&ack_response, 0, sizeof(ack_response));
    ack_response.flags = htons(ACK);
    ack_response.ack_num = htonl(incoming_packet.header.seq_num + 1);

    log_event("SND ACK FOR FIN", log_file);
    sendto(socket, &ack_response, sizeof(ack_response), 0,
           (struct sockaddr *)addr, sizeof(*addr));

    // ---- Simulate CLOSE-WAIT ----
    // Step 3: Send our FIN
    sham_header fin_response;
    memset(&fin_response, 0, sizeof(fin_response));
    uint32_t my_fin_seq = initial_seq_num + 1; // Use a predictable sequence number

    sprintf(log_msg, "SND FIN SEQ=%u", my_fin_seq);
    log_event(log_msg, log_file);

    fin_response.flags = htons(FIN);
    fin_response.seq_num = htonl(my_fin_seq);

    sendto(socket, &fin_response, sizeof(fin_response), 0,
           (struct sockaddr *)addr, sizeof(*addr));

    // Step 4: Wait for their ACK to our FIN, with retransmission
    sham_header incoming_ack;
    socklen_t addr_len = sizeof(*addr);

    // Set a timeout on the socket
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = RTO * 1000; // Use your RTO value for the timeout
    setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    int retrans_count = 0;
    while (retrans_count < 5) // Try up to 5 times
    {
        int bytes = recvfrom(socket, &incoming_ack, sizeof(incoming_ack), 0,
                             (struct sockaddr *)addr, &addr_len);

        if (bytes > 0)
        {
            incoming_ack.flags = ntohs(incoming_ack.flags);
            incoming_ack.ack_num = ntohl(incoming_ack.ack_num);

            if ((incoming_ack.flags & ACK) && incoming_ack.ack_num == my_fin_seq + 1)
            {
                sprintf(log_msg, "RCV ACK FOR FIN, ACK=%u", incoming_ack.ack_num);
                log_event(log_msg, log_file);
                printf("Connection closed cleanly.\n");

                // Reset socket to blocking before returning
                tv.tv_sec = 0;
                tv.tv_usec = 0;
                setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
                return -1; // Signal termination complete
            }
        }
        else
        {
            // Timeout occurred, retransmit our FIN
            printf("Timeout waiting for final ACK. Retransmitting FIN...\n");
            sendto(socket, &fin_response, sizeof(fin_response), 0, (struct sockaddr *)addr, sizeof(*addr));
            retrans_count++;
        }
    }

    printf("Termination failed. No final ACK received.\n");
    // Reset socket to blocking before returning
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    return 0;
    // LLM
}
// ############## LLM Generated Code Ends ##############

void check_timeouts(int socket, sliding_window *window, struct sockaddr_in *addr, char *filename)
{
    struct timeval now;
    gettimeofday(&now, NULL);

    for (int i = 0; i < sliding_size; i++)
    {
        if (window[i].in_use)
        {
            // time elapsed since sending
            long elapsed = (now.tv_sec - window[i].sent_time.tv_sec) * 1000 + (now.tv_usec - window[i].sent_time.tv_usec) / 1000;
            // whole number, and fractional parts managed separately

            if (elapsed > RTO)
            {
                // log_event("TIMEOUT SEQ=%u", window[i].packet.header.seq_num);

                // Retransmit packet
                sendto(socket, &window[i].packet, sizeof(sham_header) + window[i].actual_data_length, 0, (struct sockaddr *)addr, sizeof(*addr));
                gettimeofday(&window[i].sent_time, NULL); // Reset timer

                char log_msg[100];
                uint32_t host_seq_num = ntohl(window[i].packet.header.seq_num);
                sprintf(log_msg, "TIMEOUT SEQ=%u", host_seq_num);
                log_event(log_msg, log_file);

                sprintf(log_msg, "RETX DATA SEQ=%u LEN=%lu", host_seq_num, window[i].actual_data_length);
                log_event(log_msg, log_file);
            }
        }
    }
}

int find_free_slot(sliding_window *window)
{
    for (int i = 0; i < sliding_size; i++)
    {
        if (!window[i].in_use)
        {
            return i;
        }
    }
    return -1; // sliding window full
}

int are_there_unack(sliding_window *window)
{
    for (int i = 0; i < sliding_size; i++)
    {
        if (window[i].in_use)
        {
            return 1;
        }
    }
    return 0;
}

int send_filename(int socket, struct sockaddr_in *addr, const char *filename)
{
    sham_packet packet;
    memset(&packet, 0, sizeof(packet));

    packet.header.seq_num = htonl(current_seq_num);
    packet.header.ack_num = htonl(0);
    packet.header.flags = htons(SYN); // This is likely incorrect for a filename, consider 0
    packet.header.window_size = htons(WINDOW_SIZE);

    size_t len = strlen(filename);
    if (len >= MAXPAYLOADSIZE)
        len = MAXPAYLOADSIZE - 1;
    memcpy(packet.data, filename, len);

    int total_len = sizeof(sham_header) + len;

    int sent = sendto(socket, &packet, total_len, 0,
                      (struct sockaddr *)addr, sizeof(*addr));
    // not waiting for ack rn, not mentioned in doc
    // char log_msg[256];
    // snprintf(log_msg, sizeof(log_msg), "Sent filename: %s", filename);
    // log_event(log_msg, log_file);

    return 0;
}

int send_file(int socket, struct sockaddr_in *addr, char *filename)
{
    // socklen_t addr_len = sizeof(*addr);
    FILE *file = fopen(filename, "rb");
    if (!file)
    {
        perror("File open failed");
        return -1;
    }
    char log_msg[100];
    char arr[1024];
    int read_bytes = 0;
    sliding_window sliding_window_arr[sliding_size];
    for (int i = 0; i < sliding_size; i++)
    {
        sliding_window_arr[i].in_use = 0;
    }

    sham_header incoming_ack;
    socklen_t addr_len = sizeof(*addr);
    int file_complete = 0;
    while (!file_complete || are_there_unack(sliding_window_arr))
    {
        while (!file_complete)
        {
            if (current_seq_num + read_bytes - ack_num >= window_size)
            {
                // printf("Server Buffer full need to wait\n");
                break;
            }
            int slot = find_free_slot(sliding_window_arr);
            if (slot == -1)
            {
                // printf("slots are full rn\n");
                break;
            }

            read_bytes = fread(arr, 1, 1024, file);
            if (!read_bytes)
            {
                file_complete = 1;
                // printf("File read\n");
                break;
            }

            sham_packet to_be_sent;
            memset(&to_be_sent, 0, sizeof(to_be_sent));
            sprintf(log_msg, "SND DATA SEQ=%u LEN=%u", current_seq_num, read_bytes);
            log_event(log_msg, log_file);

            to_be_sent.header.seq_num = htonl(current_seq_num);
            to_be_sent.header.flags = htons(0);

            memcpy(to_be_sent.data, arr, read_bytes);
            sendto(socket, &to_be_sent, sizeof(sham_header) + read_bytes, 0, (struct sockaddr *)addr, sizeof(*addr)); // didn't use sizeof(sham_packet) directly cuz in that bytes read wouldn't get adjusted if less than 1024
            // printf("Read bytes %d\n", read_bytes);

            memcpy(&sliding_window_arr[slot].packet, &to_be_sent, sizeof(sham_header) + read_bytes);
            sliding_window_arr[slot].actual_data_length = read_bytes;
            sliding_window_arr[slot].in_use = 1;
            gettimeofday(&sliding_window_arr[slot].sent_time, NULL);

            current_seq_num += read_bytes;
            // log_event
        }
        int recv_bytes = recvfrom(socket, &incoming_ack, sizeof(incoming_ack), MSG_DONTWAIT, (struct sockaddr *)addr, &addr_len);

        if (recv_bytes > 0)
        {
            incoming_ack.flags = ntohs(incoming_ack.flags);
            incoming_ack.ack_num = ntohl(incoming_ack.ack_num);
            incoming_ack.window_size = ntohs(incoming_ack.window_size);

            if (incoming_ack.flags & ACK)
            {
                ack_num = incoming_ack.ack_num;
                window_size = incoming_ack.window_size;

                sprintf(log_msg, "FLOW WIN UPDATE=%u", window_size);
                log_event(log_msg, log_file);

                sprintf(log_msg, "RCV ACK=%u", ack_num);
                log_event(log_msg, log_file);

                for (int i = 0; i < sliding_size; i++)
                {
                    if (sliding_window_arr[i].in_use)
                    {
                        uint32_t packet_start_seq = ntohl(sliding_window_arr[i].packet.header.seq_num);
                        uint32_t packet_end = packet_start_seq + sliding_window_arr[i].actual_data_length;
                        if (packet_end <= ack_num)
                        {
                            sliding_window_arr[i].in_use = 0; // Free the slot
                            // printf("Packet SEQ=%u acknowledged\n", ntohl(sliding_window_arr[i].packet.header.seq_num));
                        }
                    }
                }
            }
        }
        check_timeouts(socket, sliding_window_arr, addr, filename);
        //  add delay if it hangs or doesn't receive
    }
    sham_end(socket, addr);
    fclose(file);
    return 0;
}

void print_header(sham_header to_be_printed)
{
    printf("seq_num: %u\n", to_be_printed.seq_num);
    printf("ack_num: %u\n", to_be_printed.ack_num);
    printf("flags: %u\n", to_be_printed.flags);
    printf("window_size: %u\n", to_be_printed.window_size);
}

int sham_server_accept(int client_socket, struct sockaddr_in *server_address_in)
{
    sham_header client_to_server_handshake_1;
    sham_header recieved_from_server;
    sham_header client_to_server_handshake_2;
    socklen_t server_address_len = sizeof(struct sockaddr_in);

    memset(&client_to_server_handshake_1, 0, sizeof(client_to_server_handshake_1));
    client_to_server_handshake_1.seq_num = htonl(initial_seq_num);
    client_to_server_handshake_1.flags = htons(SYN);

    if (sendto(client_socket, &client_to_server_handshake_1, sizeof(client_to_server_handshake_1), 0, (struct sockaddr *)server_address_in, server_address_len) < 0)
    {
        perror("client SYN sendto failed");
        return 1;
    }

    char log_msg[100];
    sprintf(log_msg, "SND SYN SEQ=%u", initial_seq_num);
    log_event(log_msg, log_file);

    if (recvfrom(client_socket, &recieved_from_server, sizeof(recieved_from_server), 0, (struct sockaddr *)server_address_in, &server_address_len) < 0)
    {
        perror("client ACK recvfrom failed");
        return 1;
    }

    recieved_from_server.seq_num = ntohl(recieved_from_server.seq_num);
    recieved_from_server.ack_num = ntohl(recieved_from_server.ack_num);
    recieved_from_server.flags = ntohs(recieved_from_server.flags);

    if (!(recieved_from_server.flags & SYN) || !(recieved_from_server.flags & ACK) || (recieved_from_server.ack_num != initial_seq_num + 1))
    {
        return 1;
    }

    sprintf(log_msg, "RCV ACK FOR SYN");
    log_event(log_msg, log_file);

    // print_header(client_to_server_handshake_1); // for debugging

    memset(&client_to_server_handshake_2, 0, sizeof(client_to_server_handshake_2));
    uint32_t ack_num_val = recieved_from_server.seq_num + 1;
    uint32_t my_seq_num = initial_seq_num + 1;

    sprintf(log_msg, "SND SYN-ACK SEQ=%u ACK=%u", my_seq_num, ack_num_val);
    log_event(log_msg, log_file);

    client_to_server_handshake_2.seq_num = htonl(my_seq_num);
    client_to_server_handshake_2.flags = htons(ACK);
    client_to_server_handshake_2.ack_num = htonl(ack_num_val);

    if (sendto(client_socket, &client_to_server_handshake_2, sizeof(client_to_server_handshake_2), 0, (struct sockaddr *)server_address_in, server_address_len) < 0)
    {
        perror("client acknowledgement sendto failed");
        return 1;
    }

    return 0;
}

// ############## LLM Generated Code Begins ##############
int sham_end(int socketfd, struct sockaddr_in *addr_in)
{
    socklen_t addr_len = sizeof(*addr_in);
    sham_header fin_handshake;
    char log_msg[100];

    // Step 1: Send FIN packet and prepare for retransmission
    uint32_t my_fin_seq = current_seq_num;
    
    fin_handshake.flags = htons(FIN);
    fin_handshake.seq_num = htonl(my_fin_seq);
    
    // Set a timeout for all receive calls in this function
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = RTO * 1000; // Use your RTO value
    setsockopt(socketfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    // Initially send the FIN
    sprintf(log_msg, "SND FIN SEQ=%u", my_fin_seq);
    log_event(log_msg, log_file);
    sendto(socketfd, &fin_handshake, sizeof(fin_handshake), 0, (struct sockaddr *)addr_in, sizeof(*addr_in));

    // Step 2: Wait for the ACK to our FIN, retransmitting our FIN on timeout
    int retrans_count = 0;
    while(retrans_count < 5)
    {
        if (recvfrom(socketfd, &fin_handshake, sizeof(fin_handshake), 0, (struct sockaddr *)addr_in, &addr_len) > 0) {
            fin_handshake.flags = ntohs(fin_handshake.flags);
            fin_handshake.ack_num = ntohl(fin_handshake.ack_num);

            if ((fin_handshake.flags & ACK) && fin_handshake.ack_num == my_fin_seq + 1) {
                log_event("RCV ACK FOR FIN", log_file);
                break; // This is the ACK we wanted, proceed.
            }
            // If it's not the right ACK, ignore it and loop to receive again.
        } else {
            // Timeout occurred, retransmit our FIN
            printf("Timeout waiting for FIN-ACK. Retransmitting FIN...\n");
            
            fin_handshake.flags = htons(FIN);
            fin_handshake.seq_num = htonl(my_fin_seq);
            sendto(socketfd, &fin_handshake, sizeof(fin_handshake), 0, (struct sockaddr *)addr_in, sizeof(*addr_in));
            retrans_count++;
        }
        if (retrans_count >= 5) {
             printf("Termination failed. No ACK for FIN received.\n");
             return -1;
        }
    }

    // Step 3: Wait for the Server's FIN
    while (1) {
         if (recvfrom(socketfd, &fin_handshake, sizeof(fin_handshake), 0, (struct sockaddr *)addr_in, &addr_len) > 0) {
            fin_handshake.flags = ntohs(fin_handshake.flags);
            fin_handshake.seq_num = ntohl(fin_handshake.seq_num);

            if (fin_handshake.flags & FIN) {
                sprintf(log_msg, "RCV FIN SEQ=%u", fin_handshake.seq_num);
                log_event(log_msg, log_file);
                break; // This is the FIN we wanted, proceed.
            }
             // If it's not a FIN, ignore it and loop to receive again.
         } else {
            perror("Timeout or error waiting for peer's FIN");
            return -1;
         }
    }
    
    // Step 4: Send final ACK
    uint32_t ack_for_peer = fin_handshake.seq_num + 1;
    sprintf(log_msg, "SND ACK FOR FIN, ACK=%u", ack_for_peer);
    log_event(log_msg, log_file);

    fin_handshake.flags = htons(ACK);
    fin_handshake.ack_num = htonl(ack_for_peer);
    sendto(socketfd, &fin_handshake, sizeof(fin_handshake), 0, (struct sockaddr *)addr_in, sizeof(*addr_in));

    // Reset timeout to blocking before returning to other functions
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    setsockopt(socketfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    return 0;
}
// ############## LLM Generated Code Ends ##############

int chat_mode_fn(int socket, struct sockaddr_in *addr)
{
    fd_set read_fds;
    int max_fd = socket;
    char input_buffer[1024];
    char log_msg[100];
    socklen_t addr_len = sizeof(*addr);
    sliding_window unacked_packet;
    unacked_packet.in_use = 0;
    while (1)
    {
        FD_ZERO(&read_fds);
        if (!unacked_packet.in_use)
        {
            FD_SET(STDIN_FILENO, &read_fds);
        }
        FD_SET(socket, &read_fds);

        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 200000; // check for retransmission every 200 ms

        int activity = select(max_fd + 1, &read_fds, NULL, NULL, &timeout);
        if (FD_ISSET(STDIN_FILENO, &read_fds))
        {
            if (fgets(input_buffer, sizeof(input_buffer), stdin) == NULL)
            {
                printf("Error reading from stdin\n");
                continue;
            }

            int len = strlen(input_buffer);
            if (len > 0 && input_buffer[len - 1] == '\n')
            {
                input_buffer[len - 1] = '\0';
                len--;
            } // removing newline

            if (strcmp(input_buffer, "/quit") == 0)
            {
                sham_end(socket, addr);
                return 0;
            }

            sham_packet out_packet;
            memset(&out_packet, 0, sizeof(out_packet));

            sprintf(log_msg, "SND DATA SEQ=%u LEN=%d", current_seq_num, len);
            log_event(log_msg, log_file);

            out_packet.header.seq_num = htonl(current_seq_num);
            out_packet.header.flags = htons(0);

            size_t msg_len = (len > MAXPAYLOADSIZE - 1) ? MAXPAYLOADSIZE - 1 : len;
            strncpy(out_packet.data, input_buffer, msg_len);
            out_packet.data[msg_len] = '\0';

            memcpy(&unacked_packet.packet, &out_packet, sizeof(sham_header) + msg_len);
            unacked_packet.actual_data_length = msg_len;
            unacked_packet.in_use = 1;
            gettimeofday(&unacked_packet.sent_time, NULL);
            // printf("Sending message...\n");

            int sent_bytes = sendto(socket, &out_packet, sizeof(sham_header) + msg_len, 0, (struct sockaddr *)addr, sizeof(*addr));
            if (sent_bytes < 0)
            {
                perror("chat sendto");
            }
            // current_seq_num += msg_len;
        }
        if (FD_ISSET(socket, &read_fds))
        {
            sham_packet incoming_packet;

            int recv_bytes = recvfrom(socket, &incoming_packet, sizeof(incoming_packet), 0, (struct sockaddr *)addr, &addr_len);

            if (recv_bytes > 0)
            {
                int data_len = recv_bytes - sizeof(sham_header);

                sham_header host_header = incoming_packet.header;
                host_header.seq_num = ntohl(host_header.seq_num);
                host_header.ack_num = ntohl(host_header.ack_num);
                host_header.flags = ntohs(host_header.flags);

                if (unacked_packet.in_use && data_len <= 0 && (host_header.flags & ACK))
                {
                    uint32_t expected_ack_num = ntohl(unacked_packet.packet.header.seq_num) + unacked_packet.actual_data_length;

                    if (host_header.ack_num == expected_ack_num)
                    {
                        // printf("Message delivered successfully!\n");
                        sprintf(log_msg, "RCV ACK=%u", host_header.ack_num);
                        log_event(log_msg, log_file);

                        // free the slot, ack received
                        unacked_packet.in_use = 0;
                        current_seq_num += unacked_packet.actual_data_length;
                        continue;
                    }
                }
                if (host_header.flags & FIN)
                {
                    if (sham_end_recieve(socket, incoming_packet, addr) == -1)
                        return 0;
                }

                if (data_len > 0)
                {
                    if (((double)rand() / RAND_MAX) < loss_rate)
                    {
                        sprintf(log_msg, "DROP DATA SEQ=%u", host_header.seq_num);
                        log_event(log_msg, log_file);
                        continue;
                    }

                    incoming_packet.data[data_len] = '\0'; // ensuring string is null terminated

                    sprintf(log_msg, "RCV DATA SEQ=%u LEN=%d", host_header.seq_num, data_len);
                    log_event(log_msg, log_file);

                    printf("Incoming: %s\n", incoming_packet.data);

                    sham_header ack_response;
                    memset(&ack_response, 0, sizeof(ack_response));

                    uint32_t ack_num_val = host_header.seq_num + data_len;
                    sprintf(log_msg, "SND ACK=%u WIN=%u", ack_num_val, available_buffer_size);
                    log_event(log_msg, log_file);

                    ack_response.flags = htons(ACK);
                    ack_response.ack_num = htonl(ack_num_val);
                    ack_response.window_size = htons(available_buffer_size);

                    sendto(socket, &ack_response, sizeof(ack_response), 0, (struct sockaddr *)addr, sizeof(*addr));
                }
            }
        }
        if (unacked_packet.in_use)
        {
            struct timeval now;
            gettimeofday(&now, NULL);

            long elapsed_ms = (now.tv_sec - unacked_packet.sent_time.tv_sec) * 1000 +
                              (now.tv_usec - unacked_packet.sent_time.tv_usec) / 1000;

            if (elapsed_ms > RTO)
            {
                // printf("Timeout, retransmitting message...\n");
                uint32_t host_seq_num = ntohl(unacked_packet.packet.header.seq_num);

                sprintf(log_msg, "TIMEOUT SEQ=%u", host_seq_num);
                log_event(log_msg, log_file);

                // Retransmit the stored packet
                sendto(socket, &unacked_packet.packet, sizeof(sham_header) + unacked_packet.actual_data_length, 0, (struct sockaddr *)addr, sizeof(*addr));

                sprintf(log_msg, "RETX DATA SEQ=%u LEN=%zu", host_seq_num, unacked_packet.actual_data_length);
                log_event(log_msg, log_file);

                // Reset the timer for the retransmitted packet
                gettimeofday(&unacked_packet.sent_time, NULL);
            }
        }
    }
    return 0;
}

int main(int argc, char *argv[])
{
    char *env_log = getenv("RUDP_LOG");
    if (env_log == NULL || strcmp(env_log, "1") != 0)
    {
        logging_enabled = 0;
    }
    else
    {
        logging_enabled = 1;
        log_file = fopen("client_log.txt", "w");
        if (!log_file)
        {
            perror("log_file opening failed");
        }
    }

    int chat_mode = 0;
    server_ip_address = argv[1];

    char *endptr;
    server_port = strtol(argv[2], &endptr, 10);
    if (*endptr != '\0')
    {
        {
            printf("Invalid port number\n");
            return 1;
        }
    }
    // ############## LLM Generated Code Begins ############## //LLM prompt given to change it minimally later as prev one was unrealiable
    for (int i = 3; i < argc; i++)
    {
        if (strcmp(argv[i], "--chat") == 0)
        {
            chat_mode = 1;
        }
        else
        {
            // MINIMAL CHANGE 1: Use a more reliable way to check if the argument is a number.
            char *endptr;
            float rate = strtof(argv[i], &endptr);

            // MINIMAL CHANGE 2: Check for loss_rate *before* filenames to solve the ordering issue.
            // If the entire string was a valid number, assign it to loss_rate.
            if (*endptr == '\0' && argv[i] != endptr)
            {
                loss_rate = rate;
            }
            // If it wasn't a number, it must be a filename.
            else if (!chat_mode && input_file == NULL)
            {
                input_file = argv[i];
            }
            else if (!chat_mode && output_file == NULL)
            {
                output_file = argv[i];
            }
        }
    }
    // ############## LLM Generated Code Ends ##############

    int client_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (client_socket < 0)
    {
        perror("socket formation failed");
        exit(1);
    }
    struct sockaddr_in server_address_in;
    server_address_in.sin_family = AF_INET;
    server_address_in.sin_port = htons(server_port);
    server_address_in.sin_addr.s_addr = inet_addr(server_ip_address);

    sham_server_accept(client_socket, &server_address_in);
    if (!chat_mode)
    {
        send_filename(client_socket, &server_address_in, output_file);
        send_file(client_socket, &server_address_in, input_file);
    }
    else
    {
        chat_mode_fn(client_socket, &server_address_in);
    }
    close(client_socket);
    if (log_file)
        fclose(log_file);
    return 0;
}