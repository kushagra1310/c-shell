#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include "sham.h"
int port = 8080;
const char *ip_address = "127.0.0.1"; // loopback address
int initial_seq_num = 69;             // initial seq number

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

    memset(&client_to_server_handshake_1,0,sizeof(client_to_server_handshake_1));
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
    if(!(recieved_from_server.flags & SYN) || !(recieved_from_server.flags & ACK))
    {
        return 1;
    }

    print_header(client_to_server_handshake_1); // for debugging

    memset(&client_to_server_handshake_2,0,sizeof(client_to_server_handshake_2));
    client_to_server_handshake_2.seq_num = initial_seq_num;
    client_to_server_handshake_2.flags |= ACK;
    client_to_server_handshake_2.ack_num=recieved_from_server.seq_num+1;

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
   int n = sendto(socketfd, &fin_handshake, sizeof(fin_handshake), 0, (struct sockaddr*)addr_in, sizeof(*addr_in));
   if(n < 0)
   {
       perror("sendto FIN failed");
       return -1;
   }
   printf("FIN sent\n");
   
   // Step 2: Receive ACK
   n = recvfrom(socketfd, &fin_handshake, sizeof(fin_handshake), 0, (struct sockaddr*)addr_in, &addr_len);
   if(n < 0)
   {
       perror("recvfrom ACK failed");
       return -1;
   }
   printf("ACK received\n");
   
   // Step 3: Receive FIN
   n = recvfrom(socketfd, &fin_handshake, sizeof(fin_handshake), 0, (struct sockaddr*)addr_in, &addr_len);
   if(n < 0)
   {
       perror("recvfrom FIN failed");
       return -1;
   }
   printf("FIN received\n");
   
   // Step 4: Send final ACK
   fin_handshake.flags = ACK;
   fin_handshake.ack_num = fin_handshake.seq_num + 1;
   n = sendto(socketfd, &fin_handshake, sizeof(fin_handshake), 0, (struct sockaddr*)addr_in, sizeof(*addr_in));
   if(n < 0)
   {
       perror("sendto final ACK failed");
       return -1;
   }
   printf("Final ACK sent - connection terminated\n");
   
   return 0;
}
// LLM

int main()
{
    int client_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (client_socket < 0)
    {
        perror("socket formation failed");
        exit(1);
    }
    struct sockaddr_in server_address_in;
    server_address_in.sin_family = AF_INET;
    server_address_in.sin_port = htons(port);
    server_address_in.sin_addr.s_addr = inet_addr(ip_address);

    sham_server_accept(client_socket, &server_address_in);
    return 0;
}
