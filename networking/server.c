#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "sham.h"
int port = 8080;
int backlog_size = 10;
int main()
{
    int server_socket = socket(AF_INET, SOCK_DGRAM, 0);
    int incoming_socket;
    if (server_socket < 0)
    {
        perror("socket formation failed");
        exit(1);
    }
    struct sockaddr_in client_addr_in;                  // for client address
    socklen_t client_addr_len = sizeof(client_addr_in);

    
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
    sham_header server_to_client_handshake;
    int n = recvfrom(server_socket, &server_to_client_handshake, sizeof(server_to_client_handshake), 0, (struct sockaddr *)&client_addr_in, &client_addr_len);
    printf("Message received %d\n",server_to_client_handshake.seq_num);
    close(server_socket);
    return 0;
}
