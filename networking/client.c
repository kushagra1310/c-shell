#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include "sham.h"
int port = 8080;
const char *ip_address = "127.0.0.1"; // loopback address
int initial_seq_num=69; // initial seq number
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

    sham_header client_to_server_handshake_1;
    client_to_server_handshake_1.seq_num=initial_seq_num;
    client_to_server_handshake_1.flags|=SYN;
    
    sendto(client_socket,&client_to_server_handshake_1,sizeof(client_to_server_handshake_1), 0,(struct sockaddr*)&server_address_in,sizeof(server_address_in));
    return 0;
}
