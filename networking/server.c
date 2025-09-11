#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "sham.h"
#include <openssl/evp.h>
#include <stdio.h>

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

void print_header(sham_header to_be_printed)
{
    printf("seq_num: %u\n", to_be_printed.seq_num);
    printf("ack_num: %u\n", to_be_printed.ack_num);
    printf("flags: %u\n", to_be_printed.flags);
    printf("window_size: %u\n", to_be_printed.window_size);
}

// ############## LLM Generated Code Begins ##############
int sham_end_recieve(int socket, sham_packet incoming_packet, struct sockaddr_in *addr)
{
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
    sendto(socket, &ack_response, sizeof(ack_response), 0, (struct sockaddr *)addr, sizeof(*addr));

    // ---- Simulate CLOSE-WAIT ----
    // Step 3: Send our FIN
    sham_header fin_response;
    memset(&fin_response, 0, sizeof(fin_response));
    uint32_t my_fin_seq = initial_seq_num + 1; // Use a predictable sequence number

    sprintf(log_msg, "SND FIN SEQ=%u", my_fin_seq);
    log_event(log_msg, log_file);

    fin_response.flags = htons(FIN);
    fin_response.seq_num = htonl(my_fin_seq);

    sendto(socket, &fin_response, sizeof(fin_response), 0, (struct sockaddr *)addr, sizeof(*addr));

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

    server_to_client_handshake.seq_num = ntohl(server_to_client_handshake.seq_num);
    server_to_client_handshake.flags = ntohs(server_to_client_handshake.flags);

    if (!(server_to_client_handshake.flags & SYN))
        return 1;
    char log_msg[100];
    sprintf(log_msg, "RCV SYN SEQ=%u", server_to_client_handshake.seq_num);
    log_event(log_msg, log_file);

    // printf("Message received %d\n", server_to_client_handshake.seq_num);

    uint32_t ack_num_val = server_to_client_handshake.seq_num + 1;
    sprintf(log_msg, "SND SYN-ACK SEQ=%u ACK=%u", initial_seq_num, ack_num_val);
    log_event(log_msg, log_file);

    server_to_client_handshake.ack_num = htonl(ack_num_val);
    server_to_client_handshake.seq_num = htonl(initial_seq_num);
    server_to_client_handshake.flags = htons(SYN | ACK);

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

    server_to_client_handshake.ack_num = ntohl(server_to_client_handshake.ack_num);
    server_to_client_handshake.flags = ntohs(server_to_client_handshake.flags);

    sprintf(log_msg, "RCV ACK FOR SYN");
    log_event(log_msg, log_file);

    if (!(server_to_client_handshake.flags & ACK) || (server_to_client_handshake.ack_num != initial_seq_num + 1))
        return 1;

    // if (n > 0)
    //     printf("recevied acknowledgement\n");

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
// ############## LLM Generated Code Begins ##############

void print_md5sum(const char *filename)
{
    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        perror("Error opening file");
        return;
    }

    // 1. Create a NEW context for this specific file.
    EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
    if (!mdctx) {
        fprintf(stderr, "EVP_MD_CTX_new failed\n");
        fclose(fp);
        return;
    }

    // 2. Initialize the context for an MD5 operation.
    if (EVP_DigestInit_ex(mdctx, EVP_md5(), NULL) != 1) {
        fprintf(stderr, "EVP_DigestInit_ex failed\n");
        EVP_MD_CTX_free(mdctx);
        fclose(fp);
        return;
    }

    unsigned char buffer[4096];
    size_t bytes;
    while ((bytes = fread(buffer, 1, sizeof(buffer), fp)) > 0) {
        // 3. Update the context with file data.
        if (EVP_DigestUpdate(mdctx, buffer, bytes) != 1) {
            fprintf(stderr, "EVP_DigestUpdate failed\n");
            EVP_MD_CTX_free(mdctx);
            fclose(fp);
            return;
        }
    }

    unsigned char md_value[EVP_MAX_MD_SIZE];
    unsigned int md_len;
    
    // 4. Finalize the hash. After this, the context is "spent".
    if (EVP_DigestFinal_ex(mdctx, md_value, &md_len) != 1) {
        fprintf(stderr, "EVP_DigestFinal_ex failed\n");
        EVP_MD_CTX_free(mdctx);
        fclose(fp);
        return;
    }

    // 5. Clean up the context used for this file.
    EVP_MD_CTX_free(mdctx);
    fclose(fp);

    printf("MD5: ");
    for (unsigned int i = 0; i < md_len; i++) {
        printf("%02x", md_value[i]);
    }
    printf("\n");
}
// ############## LLM Generated Code Ends ##############
// ############## LLM Generated Code Begins ##############
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
        memset(&packet, 0, sizeof(packet));
        int recv_bytes = recvfrom(socket, &packet, sizeof(packet), 0, (struct sockaddr *)addr, &addr_len);
        if (recv_bytes <= 0)
            continue;

        sham_header host_header = packet.header;
        host_header.seq_num = ntohl(host_header.seq_num);
        host_header.flags = ntohs(host_header.flags);

        int actual_size = recv_bytes - sizeof(sham_header);

        // ############## LLM Generated Code Begins ##############
        if (host_header.flags & FIN)
        {
            if (sham_end_recieve(socket, packet, addr) == -1)
            {
                fclose(file);   
                return 0;
            }
        }
        else
        {
            // ############## LLM Generated Code Ends ##############
            if (((double)rand() / RAND_MAX) < loss_rate)
            {
                char log_msg[100];
                sprintf(log_msg, "DROP DATA SEQ=%u", host_header.seq_num);
                log_event(log_msg, log_file);
                continue;
            }

            sprintf(log_msg, "RCV DATA SEQ=%u LEN=%u", host_header.seq_num, actual_size);
            log_event(log_msg, log_file);

            // check if this is the packet we're expecting
            if (host_header.seq_num == expected_seq_num)
            {
                fwrite(packet.data, 1, actual_size, file);
                // printf("Actual size %d\n", actual_size);
                // if (actual_size < MAXPAYLOADSIZE)
                // {
                //     is_it_over = 1;
                // }
                expected_seq_num += actual_size;

                int check_again = 1;

                // if we pop something from buffer, recheck if more things can be popped
                while (check_again)
                {
                    check_again = 0;

                    for (int i = 0; i < WINDOW_SIZE; i++)
                    {
                        if (buffer[i].received &&
                            ntohl(buffer[i].packet.header.seq_num) == expected_seq_num)
                        {

                            // Found the next packet in sequence!
                            // printf("Delivering buffered packet SEQ=%u\n", expected_seq_num);

                            fwrite(buffer[i].packet.data, 1, buffer[i].data_length, file);
                            // if (buffer[i].data_length < MAXPAYLOADSIZE)
                            //     is_it_over = 1;
                            expected_seq_num += buffer[i].data_length;

                            // free the buffer slot
                            buffer[i].received = 0;
                            check_again = 1;
                            break;
                        }
                    }
                }
            }
            else if (host_header.seq_num > expected_seq_num)
            {
                int already_buffered = 0;
                for (int i = 0; i < WINDOW_SIZE; i++)
                {
                    if (buffer[i].received && ntohl(buffer[i].packet.header.seq_num) == host_header.seq_num)
                    {
                        already_buffered = 1;
                        break;
                    }
                }
                // keep the packet in buffer
                if (!already_buffered)
                {
                    int i;
                    for (i = 0; i < WINDOW_SIZE; i++)
                    {
                        if (!buffer[i].received)
                        {
                            memcpy(&buffer[i].packet, &packet, sizeof(sham_packet));
                            buffer[i].data_length = actual_size;
                            buffer[i].received = 1;
                            // printf("Buffered packet SEQ=%u in slot %d\n", packet.header.seq_num, i);
                            break;
                        }
                    }
                    // if (i == WINDOW_SIZE)
                    //     printf("Buffer full\n");
                }
            }
            else
            {
                // printf("Duplicate packet received\n");
                // Buffer out-of-order packet
                // Send ACK for what we still expect
            }
            sham_header ack_packet;
            memset(&ack_packet, 0, sizeof(ack_packet));

            sprintf(log_msg, "SND ACK=%u WIN=%u", expected_seq_num, available_buffer_size);
            log_event(log_msg, log_file);

            ack_packet.flags = htons(ACK);
            ack_packet.ack_num = htonl(expected_seq_num);
            ack_packet.window_size = htons(available_buffer_size);

            sendto(socket, &ack_packet, sizeof(ack_packet), 0, (struct sockaddr *)addr, sizeof(*addr));
        }
    }
    memset(&packet, 0, sizeof(packet));
    int recv_bytes = recvfrom(socket, &packet, sizeof(packet), 0, (struct sockaddr *)addr, &addr_len);

    if (recv_bytes > 0)
    {
        int data_len = recv_bytes - sizeof(sham_header);
        packet.header.flags = ntohs(packet.header.flags);

        if (packet.header.flags & FIN)
        {
            if (sham_end_recieve(socket, packet, addr) == -1)
                return 0;
        }
    }
    fclose(file);
    return 0;
}
int recv_filename(int socket, struct sockaddr_in *addr, char *output_filename)
{
    sham_packet packet;
    socklen_t addr_len = sizeof(*addr);

    memset(&packet, 0, sizeof(packet));

    int recv_bytes = recvfrom(socket, &packet, sizeof(packet), 0, (struct sockaddr *)addr, &addr_len);

    if (recv_bytes < 0)
    {
        perror("recvfrom failed");
        return -1;
    }

    if (recv_bytes > (int)sizeof(sham_header))
    {
        int data_len = recv_bytes - sizeof(sham_header);

        // null termination
        if (data_len >= (int)sizeof(packet.data))
            data_len = sizeof(packet.data) - 1;

        memcpy(output_filename, packet.data, data_len);
        output_filename[data_len] = '\0';

        printf("Received filename: %s\n", output_filename);
    }
    else
    {
        fprintf(stderr, "Received packet too small to contain filename\n");
        return -1;
    }

    return 0;
}

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
        if (activity < 0)
        {
            perror("select error");
            continue;
        }

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
                host_header.window_size = ntohs(host_header.window_size);

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
                uint32_t retrans_seq_num = ntohl(unacked_packet.packet.header.seq_num);

                sprintf(log_msg, "TIMEOUT SEQ=%u", retrans_seq_num);
                log_event(log_msg, log_file);

                // Retransmit the stored packet
                sendto(socket, &unacked_packet.packet, sizeof(sham_header) + unacked_packet.actual_data_length, 0, (struct sockaddr *)addr, sizeof(*addr));

                sprintf(log_msg, "RETX DATA SEQ=%u LEN=%zu", retrans_seq_num, unacked_packet.actual_data_length);
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
    char output_file[1025];
    sham_client_connect(server_socket, &client_addr_in);
    if (!chat_mode)
    {
        recv_filename(server_socket, &client_addr_in, output_file);
        receive_file(server_socket, &client_addr_in, output_file);
        print_md5sum(output_file);
    }
    else
    {
        chat_mode_fn(server_socket, &client_addr_in);
    }
    close(server_socket);
    if (log_file)
        fclose(log_file);
    return 0;
}
