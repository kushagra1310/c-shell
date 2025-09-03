#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
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
                sendto(socket, &window[i].packet, sizeof(sham_header) + window[i].actual_data_length, 0, (struct sockaddr*)addr, sizeof(*addr));
                gettimeofday(&window[i].sent_time, NULL); // Reset timer

                // log_event("RETX DATA SEQ=%u LEN=...", window[i].packet.header.seq_num);
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

int send_file(int socket, struct sockaddr_in *addr, char *filename)
{
    // socklen_t addr_len = sizeof(*addr);
    FILE *file = fopen(filename, "rb");
    if (!file)
    {
        perror("File open failed");
        return -1;
    }
    char arr[1024];
    int read_bytes = 0;
    sliding_window sliding_window_arr[sliding_size];
    for (int i = 0; i < sliding_size; i++)
    {
        sliding_window_arr[i].in_use = 0;
    }

    sham_header incoming_ack;
    socklen_t addr_len = sizeof(*addr);
    while ((read_bytes = fread(arr, 1, 1024, file)) > 0)
    {
        sham_packet to_be_sent;
        to_be_sent.header.seq_num = current_seq_num;
        to_be_sent.header.flags = 0;
        memcpy(to_be_sent.data, arr, read_bytes);
        printf("Read bytes %d\n",read_bytes);
        int slot = find_free_slot(sliding_window_arr);
        if (slot != -1)
        {
            sliding_window_arr[slot].packet.header.seq_num = current_seq_num;
            sliding_window_arr[slot].packet.header.flags = 0;
            sliding_window_arr[slot].actual_data_length = read_bytes;
            memcpy(sliding_window_arr[slot].packet.data, to_be_sent.data, sizeof(sham_header) + read_bytes);
            if (current_seq_num + read_bytes - ack_num <= window_size)
            {
                sendto(socket, &to_be_sent, sizeof(sham_header) + read_bytes, 0, (struct sockaddr*)addr, sizeof(*addr)); // didn't use sizeof(sham_packet) directly cuz in that bytes read wouldn't get adjusted if less than 1024
                sliding_window_arr[slot].in_use = 1;

                gettimeofday(&sliding_window_arr[slot].sent_time, NULL);
                current_seq_num += read_bytes;
                // log_event
            }
        }

        int recv_bytes = recvfrom(socket, &incoming_ack, sizeof(incoming_ack), MSG_DONTWAIT, (struct sockaddr *)addr, &addr_len);

        if (recv_bytes > 0 && (incoming_ack.flags & ACK))
        {
            ack_num = incoming_ack.ack_num;
            window_size = incoming_ack.window_size;
        }

        for (int i = 0; i < WINDOW_SIZE; i++)
        {
            if (sliding_window_arr[i].in_use)
            {
                uint32_t packet_end = sliding_window_arr[i].packet.header.seq_num + sliding_window_arr[i].actual_data_length;
                if (packet_end <= ack_num)
                {
                    sliding_window_arr[i].in_use = 0; // Free the slot
                    printf("Packet SEQ=%u acknowledged\n", sliding_window_arr[i].packet.header.seq_num);
                }
            }
        }

        check_timeouts(socket, sliding_window_arr, addr, filename);
    }

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
    client_to_server_handshake_1.seq_num = initial_seq_num;
    client_to_server_handshake_1.flags |= SYN;

    if (sendto(client_socket, &client_to_server_handshake_1, sizeof(client_to_server_handshake_1), 0, (struct sockaddr *)server_address_in, server_address_len) < 0)
    {
        perror("client SYN sendto failed");
        return 1;
    }
    if (recvfrom(client_socket, &recieved_from_server, sizeof(recieved_from_server), 0, (struct sockaddr *)server_address_in, &server_address_len) < 0)
    {
        perror("client ACK recvfrom failed");
        return 1;
    }
    if (!(recieved_from_server.flags & SYN) || !(recieved_from_server.flags & ACK) || (recieved_from_server.ack_num != initial_seq_num + 1))
    {
        return 1;
    }

    print_header(client_to_server_handshake_1); // for debugging

    memset(&client_to_server_handshake_2, 0, sizeof(client_to_server_handshake_2));
    client_to_server_handshake_2.seq_num = initial_seq_num;
    client_to_server_handshake_2.flags |= ACK;
    client_to_server_handshake_2.ack_num = recieved_from_server.seq_num + 1;

    if (sendto(client_socket, &client_to_server_handshake_2, sizeof(client_to_server_handshake_2), 0, (struct sockaddr *)server_address_in, server_address_len) < 0)
    {
        perror("client acknowledgement sendto failed");
        return 1;
    }

    return 0;
}
// LLM
int sham_end(int socketfd, struct sockaddr_in *addr_in)
{
    socklen_t addr_len = sizeof(*addr_in);

    sham_header fin_handshake;

    // Step 1: Send FIN packet
    fin_handshake.flags = FIN;
    fin_handshake.seq_num = initial_seq_num;
    int n = sendto(socketfd, &fin_handshake, sizeof(fin_handshake), 0, (struct sockaddr *)addr_in, sizeof(*addr_in));
    if (n < 0)
    {
        perror("sendto FIN failed");
        return -1;
    }
    printf("FIN sent\n");

    // Step 2: Receive ACK
    n = recvfrom(socketfd, &fin_handshake, sizeof(fin_handshake), 0, (struct sockaddr *)addr_in, &addr_len);
    if (n < 0)
    {
        perror("recvfrom ACK failed");
        return -1;
    }
    printf("ACK received\n");

    // Step 3: Receive FIN
    n = recvfrom(socketfd, &fin_handshake, sizeof(fin_handshake), 0, (struct sockaddr *)addr_in, &addr_len);
    if (n < 0)
    {
        perror("recvfrom FIN failed");
        return -1;
    }
    printf("FIN received\n");

    // Step 4: Send final ACK
    fin_handshake.flags = ACK;
    fin_handshake.ack_num = fin_handshake.seq_num + 1;
    n = sendto(socketfd, &fin_handshake, sizeof(fin_handshake), 0, (struct sockaddr *)addr_in, sizeof(*addr_in));
    if (n < 0)
    {
        perror("sendto final ACK failed");
        return -1;
    }
    printf("Final ACK sent - connection terminated\n");

    return 0;
}
// LLM

int main(int argc, char *argv[])
{
    int chat_mode = 0;
    float loss_rate = 0.0;
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "--chat") == 0)
        {
            chat_mode = 1;
        }
        else
        {
            loss_rate = atof(argv[i]); // Assume it's loss rate
        }
    }

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
    if (chat_mode)
    {
        loss_rate = strtof(argv[4], NULL);
    }
    else if (argc > 4)
    {
        input_file = argv[3];
        output_file = argv[4];
    }

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
    send_file(client_socket, &server_address_in, input_file);
    return 0;
}
