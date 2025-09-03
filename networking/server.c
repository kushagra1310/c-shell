#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "sham.h"

long port;
int backlog_size = 10;
int initial_seq_num = 1;
int expected_seq_num = 1;
int available_buffer_size = WINDOW_SIZE * MAXPAYLOADSIZE;
int sham_client_connect(int server_socket, struct sockaddr_in *client_addr_in)
{
    socklen_t client_addr_len = sizeof(*client_addr_in);

    sham_header server_to_client_handshake;
    int n = recvfrom(server_socket, &server_to_client_handshake, sizeof(server_to_client_handshake), 0, (struct sockaddr *)client_addr_in, &client_addr_len);
    if (n < 0)
    {
        perror("recvfrom failed");
        return 1;
    }
    if (!(server_to_client_handshake.flags & SYN))
        return 1;

    printf("Message received %d\n", server_to_client_handshake.seq_num);

    server_to_client_handshake.ack_num = server_to_client_handshake.seq_num + 1;
    server_to_client_handshake.seq_num = initial_seq_num;
    server_to_client_handshake.flags |= ACK;

    if (sendto(server_socket, &server_to_client_handshake, sizeof(server_to_client_handshake), 0, (struct sockaddr *)client_addr_in, sizeof(*client_addr_in)) < 0)
    {
        perror("server ACK sendto failed");
        return 1;
    }

    if (recvfrom(server_socket, &server_to_client_handshake, sizeof(server_to_client_handshake), 0, (struct sockaddr *)client_addr_in, &client_addr_len) < 0)
    {
        perror("server ACK recvfrom failed");
        return 1;
    }

    if (!(server_to_client_handshake.flags & ACK) || (server_to_client_handshake.ack_num != initial_seq_num + 1))
        return 1;

    if (n > 0)
        printf("recevied acknowledgement\n");

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

int receive_file(int socket, struct sockaddr_in *addr, char *output_filename)
{
    FILE *file = fopen(output_filename, "wb");
    socklen_t addr_len = sizeof(*addr);

    // buffer for out-of-order packets
    receive_buffer_entry buffer[WINDOW_SIZE];
    for (int i = 0; i < WINDOW_SIZE; i++)
    {
        buffer[i].received = 0;
    }

    // int expected_seq = expected_seq_num;
    int is_it_over = 0;
    while (!is_it_over)
    {
        sham_packet packet;
        int recv_bytes = recvfrom(socket, &packet, sizeof(packet), 0, (struct sockaddr*)addr, &addr_len);
        if (recv_bytes <= 0)
            continue;
        int actual_size = recv_bytes - sizeof(sham_header);
        // Check if this is the packet we're expecting
        if (packet.header.seq_num == expected_seq_num)
        {
            fwrite(packet.data, 1, actual_size, file);
            printf("Actual size %d\n",actual_size);
            if (actual_size < MAXPAYLOADSIZE)
            {
                is_it_over = 1;
            }
            expected_seq_num += actual_size;

            int check_again = 1;

            // if we pop something from buffer, recheck if more things can be popped
            while (check_again)
            {
                check_again = 0;

                for (int i = 0; i < WINDOW_SIZE; i++)
                {
                    if (buffer[i].received &&
                        buffer[i].packet.header.seq_num == expected_seq_num)
                    {

                        // Found the next packet in sequence!
                        printf("Delivering buffered packet SEQ=%u\n", expected_seq_num);

                        fwrite(buffer[i].packet.data, 1, buffer[i].data_length, file);
                        if (buffer[i].data_length < MAXPAYLOADSIZE)
                            is_it_over = 1;
                        expected_seq_num += buffer[i].data_length;

                        // free the buffer slot
                        buffer[i].received = 0;
                        check_again = 1;
                        break;
                    }
                }
            }
        }
        else if (packet.header.seq_num > expected_seq_num)
        {
            // keep the packet in buffer
            int i;
            for (i = 0; i < WINDOW_SIZE; i++)
            {
                if (!buffer[i].received)
                {
                    memcpy(&buffer[i].packet, &packet, sizeof(sham_packet));
                    buffer[i].data_length = actual_size;
                    buffer[i].received = 1;
                    printf("Buffered packet SEQ=%u in slot %d\n", packet.header.seq_num, i);
                }
            }
            if (i == WINDOW_SIZE)
                printf("Buffer full\n");
        }
        else
        {
            printf("Duplicate packet received\n");
            // Buffer out-of-order packet
            // Send ACK for what we still expect
        }
        sham_header ack_packet;
        memset(&ack_packet, 0, sizeof(ack_packet));

        ack_packet.flags = ACK;
        ack_packet.ack_num = expected_seq_num;
        ack_packet.window_size = available_buffer_size;

        sendto(socket, &ack_packet, sizeof(ack_packet), 0, (struct sockaddr *)addr, sizeof(*addr));
    }

    fclose(file);
    return 0;
}

int main(int argc, char *argv[])
{
    int chat_mode = 0;
    float loss_rate = 0.0;
    for (int i = 2; i < argc; i++)
    {
        if (strcmp(argv[i], "--chat") == 0)
        {
            chat_mode = 1;
        }
        else
        {
            char *end_ptr;
            loss_rate = strtof(argv[i], &end_ptr);
            if (*end_ptr != '\0')
            {
                printf("Invalid loss rate\n");
            }
        }
    }
    char *endptr;
    port = strtol(argv[1], &endptr, 10);
    if (*endptr != '\0')
    {
        {
            printf("Invalid port number\n");
            return 1;
        }
    }

    int server_socket = socket(AF_INET, SOCK_DGRAM, 0);
    int incoming_socket;
    if (server_socket < 0)
    {
        perror("socket formation failed");
        exit(1);
    }
    struct sockaddr_in client_addr_in; // for client address
    struct sockaddr_in server_addr_in;

    memset(&server_addr_in, 0, sizeof(server_addr_in)); // clearing the struct so that no garbage values
    server_addr_in.sin_family = AF_INET;
    server_addr_in.sin_port = htons(port);              // port number after convert byte order of host to network little endian to big endian etc
    server_addr_in.sin_addr.s_addr = htonl(INADDR_ANY); // accept any incoming connection from local IPs

    if (bind(server_socket, (struct sockaddr *)&server_addr_in, sizeof(server_addr_in)) < 0)
    {
        perror("bind failed");
        exit(1);
    }

    sham_client_connect(server_socket, &client_addr_in);
    if(!chat_mode)
    receive_file(server_socket, &client_addr_in, "received_file.txt");
    close(server_socket);
    return 0;
}
