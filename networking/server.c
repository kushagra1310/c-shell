#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <openssl/evp.h>
#include "sham.h"

long port;
int backlog_size = 10;
int initial_seq_num = 1;
int current_seq_num = 1;
int expected_seq_num = 1;
int available_buffer_size = WINDOW_SIZE * MAXPAYLOADSIZE;
FILE *log_file = NULL;
int logging_enabled = 0;
float loss_rate;
int chat_mode;

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
    char log_msg[100];
    sprintf(log_msg, "RCV SYN SEQ=%u", server_to_client_handshake.seq_num);
    log_event(log_msg, log_file);

    printf("Message received %d\n", server_to_client_handshake.seq_num);

    server_to_client_handshake.ack_num = server_to_client_handshake.seq_num + 1;
    server_to_client_handshake.seq_num = initial_seq_num;
    server_to_client_handshake.flags |= ACK;

    if (sendto(server_socket, &server_to_client_handshake, sizeof(server_to_client_handshake), 0, (struct sockaddr *)client_addr_in, sizeof(*client_addr_in)) < 0)
    {
        perror("server ACK sendto failed");
        return 1;
    }

    sprintf(log_msg, "SND SYN-ACK SEQ=%u ACK=%u", server_to_client_handshake.seq_num, server_to_client_handshake.ack_num);
    log_event(log_msg, log_file);

    if (recvfrom(server_socket, &server_to_client_handshake, sizeof(server_to_client_handshake), 0, (struct sockaddr *)client_addr_in, &client_addr_len) < 0)
    {
        perror("server ACK recvfrom failed");
        return 1;
    }

    sprintf(log_msg, "RCV ACK FOR SYN");
    log_event(log_msg, log_file);

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
// LLM
#include <openssl/evp.h>
#include <stdio.h>

void print_md5sum(const char *filename)
{
    FILE *fp = fopen(filename, "rb");
    if (!fp)
    {
        perror("fopen");
        return;
    }

    EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
    if (!mdctx)
    {
        fprintf(stderr, "EVP_MD_CTX_new failed\n");
        fclose(fp);
        return;
    }

    if (EVP_DigestInit_ex(mdctx, EVP_md5(), NULL) != 1)
    {
        fprintf(stderr, "EVP_DigestInit_ex failed\n");
        EVP_MD_CTX_free(mdctx);
        fclose(fp);
        return;
    }

    unsigned char buffer[4096];
    size_t bytes;
    while ((bytes = fread(buffer, 1, sizeof(buffer), fp)) > 0)
    {
        if (EVP_DigestUpdate(mdctx, buffer, bytes) != 1)
        {
            fprintf(stderr, "EVP_DigestUpdate failed\n");
            EVP_MD_CTX_free(mdctx);
            fclose(fp);
            return;
        }
    }

    unsigned char md_value[EVP_MAX_MD_SIZE];
    unsigned int md_len;
    if (EVP_DigestFinal_ex(mdctx, md_value, &md_len) != 1)
    {
        fprintf(stderr, "EVP_DigestFinal_ex failed\n");
        EVP_MD_CTX_free(mdctx);
        fclose(fp);
        return;
    }

    EVP_MD_CTX_free(mdctx);
    fclose(fp);

    // Print in required format
    printf("MD5: ");
    for (unsigned int i = 0; i < md_len; i++)
    {
        printf("%02x", md_value[i]);
    }
    printf("\n");
}

// LLM
int receive_file(int socket, struct sockaddr_in *addr, char *output_filename)
{
    char log_msg[100];
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
    sham_packet packet;
    while (!is_it_over)
    {
        int recv_bytes = recvfrom(socket, &packet, sizeof(packet), 0, (struct sockaddr *)addr, &addr_len);
        if (recv_bytes <= 0)
            continue;
        int actual_size = recv_bytes - sizeof(sham_header);

        double random_value = (double)rand() / RAND_MAX;

        if (random_value < loss_rate)
        {
            char log_msg[100];
            sprintf(log_msg, "DROP DATA SEQ=%u", packet.header.seq_num);
            log_event(log_msg, log_file);
            continue;
        }

        // LLM
        if (packet.header.flags & FIN)
        {
            char log_msg[100];
            sprintf(log_msg, "RCV FIN SEQ=%u", packet.header.seq_num);
            log_event(log_msg, log_file);

            // Send ACK for the FIN
            sham_header ack_for_fin;
            memset(&ack_for_fin, 0, sizeof(ack_for_fin));
            ack_for_fin.flags = ACK;
            ack_for_fin.ack_num = packet.header.seq_num + 1;
            ack_for_fin.window_size = available_buffer_size;
            sendto(socket, &ack_for_fin, sizeof(ack_for_fin), 0, (struct sockaddr *)addr, sizeof(*addr));

            sprintf(log_msg, "SND ACK FOR FIN");
            log_event(log_msg, log_file);

            is_it_over = 1; // Signal to exit the file receiving loop
            continue;       // Skip the rest of the data processing logic
        }
        // LLM

        sprintf(log_msg, "RCV DATA SEQ=%u LEN=%u", packet.header.seq_num, actual_size);
        log_event(log_msg, log_file);

        // check if this is the packet we're expecting
        if (packet.header.seq_num == expected_seq_num)
        {
            fwrite(packet.data, 1, actual_size, file);
            printf("Actual size %d\n", actual_size);
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
        sprintf(log_msg, "SND ACK=%u WIN=%u", ack_packet.ack_num, ack_packet.window_size);
        log_event(log_msg, log_file);
    }
    int recv_bytes = recvfrom(socket, &packet, sizeof(packet), 0, (struct sockaddr *)addr, &addr_len);

    if (recv_bytes > 0)
    {
        int data_len = recv_bytes - sizeof(sham_header);
        // LLM
        if (packet.header.flags & FIN)
        {
            // Step 1: Receive FIN
            sprintf(log_msg, "RCV FIN SEQ=%u", packet.header.seq_num);
            log_event(log_msg, log_file);

            printf("Other side initiated disconnect. Beginning 4-way termination...\n");

            // Step 2: Send ACK for their FIN
            sham_header ack_response;
            memset(&ack_response, 0, sizeof(ack_response));
            ack_response.flags = ACK;
            ack_response.ack_num = packet.header.seq_num + 1;

            log_event("SND ACK FOR FIN", log_file);
            sendto(socket, &ack_response, sizeof(ack_response), 0,
                   (struct sockaddr *)addr, sizeof(*addr));

            // ---- Simulate CLOSE-WAIT ----
            // At this point, your application decides it's done and sends its own FIN.

            sham_header fin_response;
            memset(&fin_response, 0, sizeof(fin_response));
            fin_response.flags = FIN;
            fin_response.seq_num = current_seq_num++; // track your own SEQ numbers

            sprintf(log_msg, "SND FIN SEQ=%u", fin_response.seq_num);
            log_event(log_msg, log_file);

            sendto(socket, &fin_response, sizeof(fin_response), 0,
                   (struct sockaddr *)addr, sizeof(*addr));

            // Step 4: Wait for their ACK to our FIN
            sham_header incoming_ack;
            socklen_t addr_len = sizeof(*addr);
            int bytes = recvfrom(socket, &incoming_ack, sizeof(incoming_ack), 0,
                                 (struct sockaddr *)addr, &addr_len);

            if (bytes > 0 && (incoming_ack.flags & ACK) &&
                incoming_ack.ack_num == fin_response.seq_num + 1)
            {
                sprintf(log_msg, "RCV ACK FOR FIN, ACK=%u", incoming_ack.ack_num);
                log_event(log_msg, log_file);

                printf("Connection closed cleanly.\n");
                return -1; // signal termination complete
            }
        }
    }
    fclose(file);
    return 0;
}
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
            current_seq_num++;
        }
        if (FD_ISSET(socket, &read_fds))
        {
            sham_packet incoming_packet;

            int recv_bytes = recvfrom(socket, &incoming_packet, sizeof(incoming_packet), 0, (struct sockaddr *)addr, &addr_len);

            if (recv_bytes > 0)
            {
                int data_len = recv_bytes - sizeof(sham_header);
                // LLM
                if (incoming_packet.header.flags & FIN)
                {
                    // Step 1: Receive FIN
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

                    // ---- Simulate CLOSE-WAIT ----
                    // At this point, your application decides it's done and sends its own FIN.

                    sham_header fin_response;
                    memset(&fin_response, 0, sizeof(fin_response));
                    fin_response.flags = FIN;
                    fin_response.seq_num = current_seq_num++; // track your own SEQ numbers

                    sprintf(log_msg, "SND FIN SEQ=%u", fin_response.seq_num);
                    log_event(log_msg, log_file);

                    sendto(socket, &fin_response, sizeof(fin_response), 0,
                           (struct sockaddr *)addr, sizeof(*addr));

                    // Step 4: Wait for their ACK to our FIN
                    sham_header incoming_ack;
                    socklen_t addr_len = sizeof(*addr);
                    int bytes = recvfrom(socket, &incoming_ack, sizeof(incoming_ack), 0,
                                         (struct sockaddr *)addr, &addr_len);

                    if (bytes > 0 && (incoming_ack.flags & ACK) &&
                        incoming_ack.ack_num == fin_response.seq_num + 1)
                    {
                        sprintf(log_msg, "RCV ACK FOR FIN, ACK=%u", incoming_ack.ack_num);
                        log_event(log_msg, log_file);

                        printf("Connection closed cleanly.\n");
                        return -1; // signal termination complete
                    }
                }

                // LLM
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
                    ack_response.window_size = available_buffer_size;

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
    srand(time(NULL)); // LLM used. Seed the random generator
    char *env_log = getenv("RUDP_LOG");
    if (env_log == NULL || strcmp(env_log, "1") != 0)
    {
        logging_enabled = 0;
    }
    else
    {
        logging_enabled = 1;
        log_file = fopen("server_log.txt", "w");
        if (!log_file)
        {
            perror("log_file opening failed");
        }
    }

    chat_mode = 0;
    loss_rate = 0.0;
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
    if (!chat_mode)
    {
        receive_file(server_socket, &client_addr_in, "received_file.txt");
        print_md5sum("received_file.txt");
    }
    else
    {
        if (!chat_mode_fn(server_socket, &client_addr_in))
        {
            sham_end(server_socket, &client_addr_in);
        }
    }
    close(server_socket);
    if (log_file)
        fclose(log_file);
    return 0;
}
