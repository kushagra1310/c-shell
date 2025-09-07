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

int sham_end_receive(int socket, sham_packet incoming_packet, struct sockaddr_in *addr)
{
    // ############## LLM Generated Code Begins ##############
    char log_msg[100];
    // Step 1: Peer wants to close
    sprintf(log_msg, "RCV FIN SEQ=%u", incoming_packet.header.seq_num);
    log_event(log_msg, log_file);

    printf("Other side initiated disconnect. Beginning 4-way termination...\n");

    // Step 2: Send ACK for their FIN
    sham_header ack_response;
    memset(&ack_response, 0, sizeof(ack_response));
    ack_response.flags = ACK;
    ack_response.ack_num = incoming_packet.header.seq_num + 1;

    log_event("SND ACK FOR FIN", log_file);
    sendto(socket, &ack_response, sizeof(ack_response), 0,
           (struct sockaddr *)addr, sizeof(*addr));

    // ---- At this point, we are in CLOSE-WAIT state ----
    // Application decides it's done, so we initiate our FIN.

    // Step 3: Send our FIN
    sham_header fin_response;
    memset(&fin_response, 0, sizeof(fin_response));
    fin_response.flags = FIN;
    fin_response.seq_num = current_seq_num++; // use your seq num tracker

    sprintf(log_msg, "SND FIN SEQ=%u", fin_response.seq_num);
    log_event(log_msg, log_file);

    sendto(socket, &fin_response, sizeof(fin_response), 0,
           (struct sockaddr *)addr, sizeof(*addr));

    // ---- Now we wait for their ACK to our FIN ----
    sham_header incoming_ack;
    socklen_t addr_len = sizeof(*addr);
    int bytes = recvfrom(socket, &incoming_ack, sizeof(incoming_ack), 0,
                         (struct sockaddr *)addr, &addr_len);

    if (bytes > 0 && (incoming_ack.flags & ACK) &&
        incoming_ack.ack_num == fin_response.seq_num + 1)
    {
        // Step 4: Got their ACK to our FIN -> Connection terminated
        sprintf(log_msg, "RCV ACK FOR FIN, ACK=%u", incoming_ack.ack_num);
        log_event(log_msg, log_file);

        printf("Connection closed cleanly.\n");
        return -1; // signal termination
    }

    // ############## LLM Generated Code Ends ##############
}

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
                sprintf(log_msg, "TIMEOUT SEQ=%u", window[i].packet.header.seq_num);
                log_event(log_msg, log_file);

                sprintf(log_msg, "RETX DATA SEQ=%u LEN=%lu", window[i].packet.header.seq_num, window[i].actual_data_length);
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
                printf("Server Buffer full need to wait\n");
                break;
            }
            int slot = find_free_slot(sliding_window_arr);
            if (slot == -1)
            {
                printf("slots are full rn\n");
                break;
            }

            read_bytes = fread(arr, 1, 1024, file);
            if (!read_bytes)
            {
                file_complete = 1;
                printf("File read\n");
                break;
            }

            sham_packet to_be_sent;
            to_be_sent.header.seq_num = current_seq_num;
            to_be_sent.header.flags = 0;

            memcpy(to_be_sent.data, arr, read_bytes);
            sendto(socket, &to_be_sent, sizeof(sham_header) + read_bytes, 0, (struct sockaddr *)addr, sizeof(*addr)); // didn't use sizeof(sham_packet) directly cuz in that bytes read wouldn't get adjusted if less than 1024
            // printf("Read bytes %d\n", read_bytes);

            sliding_window_arr[slot].packet.header.seq_num = current_seq_num;
            sliding_window_arr[slot].packet.header.flags = 0;
            sliding_window_arr[slot].actual_data_length = read_bytes;
            memcpy(sliding_window_arr[slot].packet.data, to_be_sent.data, read_bytes);

            sliding_window_arr[slot].in_use = 1;
            gettimeofday(&sliding_window_arr[slot].sent_time, NULL);

            sprintf(log_msg, "SND DATA SEQ=%u LEN=%u", to_be_sent.header.seq_num, read_bytes);
            log_event(log_msg, log_file);
            current_seq_num += read_bytes;
            // log_event
        }
        int recv_bytes = recvfrom(socket, &incoming_ack, sizeof(incoming_ack), MSG_DONTWAIT, (struct sockaddr *)addr, &addr_len);

        if (recv_bytes > 0 && (incoming_ack.flags & ACK))
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
                    uint32_t packet_end = sliding_window_arr[i].packet.header.seq_num + sliding_window_arr[i].actual_data_length;
                    if (packet_end <= ack_num)
                    {
                        sliding_window_arr[i].in_use = 0; // Free the slot
                        printf("Packet SEQ=%u acknowledged\n", sliding_window_arr[i].packet.header.seq_num);
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
    client_to_server_handshake_1.seq_num = initial_seq_num;
    client_to_server_handshake_1.flags |= SYN;

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

    if (!(recieved_from_server.flags & SYN) || !(recieved_from_server.flags & ACK) || (recieved_from_server.ack_num != initial_seq_num + 1))
    {
        return 1;
    }

    sprintf(log_msg, "RCV ACK FOR SYN");
    log_event(log_msg, log_file);

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

    sprintf(log_msg, "SND SYN-ACK SEQ=%u ACK=%u", initial_seq_num, client_to_server_handshake_2.ack_num);
    log_event(log_msg, log_file);

    return 0;
}

// LLM
int sham_end(int socketfd, struct sockaddr_in *addr_in)
{
    socklen_t addr_len = sizeof(*addr_in);
    sham_header fin_handshake;
    char log_msg[100]; // Buffer for log messages

    // Step 1: Send FIN packet
    fin_handshake.flags = FIN;
    fin_handshake.seq_num = current_seq_num;

    sprintf(log_msg, "SND FIN SEQ=%u", fin_handshake.seq_num);
    log_event(log_msg, log_file);

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

    log_event("SND ACK FOR FIN", log_file);
    printf("ACK received\n");

    // Step 3: Receive FIN
    n = recvfrom(socketfd, &fin_handshake, sizeof(fin_handshake), 0, (struct sockaddr *)addr_in, &addr_len);
    if (n < 0)
    {
        perror("recvfrom FIN failed");
        return -1;
    }

    sprintf(log_msg, "RCV FIN SEQ=%u", fin_handshake.seq_num);
    log_event(log_msg, log_file);
    printf("FIN received\n");

    // Step 4: Send final ACK
    fin_handshake.flags = ACK;
    fin_handshake.ack_num = fin_handshake.seq_num + 1;

    log_event("SND ACK FOR FIN", log_file);

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

int chat_mode_fn(int socket, struct sockaddr_in *addr)
{
    fd_set read_fds;
    int max_fd = -1;
    char input_buffer[1024];
    char log_msg[100];
    socklen_t addr_len = sizeof(*addr);

    while (1)
    {
        FD_ZERO(&read_fds);
        FD_SET(STDIN_FILENO, &read_fds);
        max_fd = (max_fd > STDIN_FILENO) ? max_fd : STDIN_FILENO;
        FD_SET(socket, &read_fds);
        max_fd = (max_fd > socket) ? max_fd : socket;

        int activity = select(max_fd + 1, &read_fds, NULL, NULL, NULL);
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
                return 0;
            }

            sham_packet out_packet;
            memset(&out_packet, 0, sizeof(out_packet));
            out_packet.header.seq_num = current_seq_num;
            out_packet.header.flags = 0;

            size_t msg_len = (len > MAXPAYLOADSIZE - 1) ? MAXPAYLOADSIZE - 1 : len;
            strncpy(out_packet.data, input_buffer, msg_len);
            out_packet.data[msg_len] = '\0';

            int sent_bytes = sendto(socket, &out_packet, sizeof(sham_header) + msg_len, 0, (struct sockaddr *)addr, sizeof(*addr));
            if (sent_bytes >= 0)
            {
                sprintf(log_msg, "SND DATA SEQ=%u LEN=%zu", current_seq_num, msg_len);
                log_event(log_msg, log_file);
            }
        }
        if (FD_ISSET(socket, &read_fds))
        {
            sham_packet incoming_packet;

            int recv_bytes = recvfrom(socket, &incoming_packet, sizeof(incoming_packet), 0, (struct sockaddr *)addr, &addr_len);

            if (recv_bytes > 0)
            {
                int data_len = recv_bytes - sizeof(sham_header);
                // ############## LLM Generated Code Begins ##############
                if (incoming_packet.header.flags & FIN)
                {
                    sham_end_receive(socket, incoming_packet,addr);
                }
                // ############## LLM Generated Code Ends ##############

                if (data_len > 0)
                {
                    incoming_packet.data[data_len] = '\0'; // ensuring string is null terminated

                    sprintf(log_msg, "RCV DATA SEQ=%u LEN=%d", incoming_packet.header.seq_num, data_len);
                    log_event(log_msg, log_file);

                    printf("Incoming: %s\n", incoming_packet.data);

                    sham_header ack_response;
                    memset(&ack_response, 0, sizeof(ack_response));
                    ack_response.flags = ACK;
                    ack_response.ack_num = incoming_packet.header.seq_num + data_len;
                    ack_response.window_size = window_size; // buffer size not specified in document

                    sendto(socket, &ack_response, sizeof(ack_response), 0, (struct sockaddr *)addr, sizeof(*addr));
                    sprintf(log_msg, "SND ACK=%u WIN=%u", ack_response.ack_num, ack_response.window_size);
                    log_event(log_msg, log_file);
                }
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
    float loss_rate = 0.0;
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
    // LLM
    for (int i = 3; i < argc; i++)
    {
        if (strcmp(argv[i], "--chat") == 0)
        {
            chat_mode = 1;
        }
        else if (strstr(argv[i], ".") != NULL && atof(argv[i]) > 0)
        {
            loss_rate = atof(argv[i]);
        }
        else if (!chat_mode && input_file == NULL)
        {
            input_file = argv[i];
        }
        else if (!chat_mode && output_file == NULL)
        {
            output_file = argv[i];
        }
    }
    // LLM

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
        send_file(client_socket, &server_address_in, input_file);
    else
    {
        if (!chat_mode_fn(client_socket, &server_address_in))
        {
            sham_end(client_socket, &server_address_in);
        }
    }
    close(client_socket);
    if (log_file)
        fclose(log_file);
    return 0;
}
