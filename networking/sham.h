#include <stdio.h>
#include <stdint.h>
#include <sys/time.h>
#define SYN 0x1
#define ACK 0x2
#define FIN 0x4
#define MAXPAYLOADSIZE 1024
#define WINDOW_SIZE 10
#define RTO 500

typedef struct
{
    uint32_t s_addr; // IPv4 address for abstraction purposes
} in_addr;


typedef struct
{
    uint32_t seq_num;     // Sequence Number
    uint32_t ack_num;     // Acknowledgment Number
    uint16_t flags;       // Control flags (SYN, ACK, FIN)
    uint16_t window_size; // Flow control window size
} sham_header;

typedef struct
{
    sham_header header;
    char data[MAXPAYLOADSIZE];
} sham_packet;

typedef struct
{
    sham_packet packet;
    struct timeval sent_time; // to record time sent
    int in_use; // to record whether it is in use or not
    size_t actual_data_length; // length of actual data
} sliding_window;
typedef struct {
    sham_packet packet;
    size_t data_length;
    int received;  // 1 if this packet has arrived, 0 if not
} receive_buffer_entry;
int sham_end(int socketfd, struct sockaddr_in *addr_in);
int send_file(int socket, struct sockaddr_in *addr, char *filename);
int receive_file(int socket, struct sockaddr_in *addr, char *output_filename);
// void log_event(const char *format);
