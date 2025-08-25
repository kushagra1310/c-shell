#include <stdio.h>
#include <stdint.h>
#define SYN 0x1
#define ACK 0x2
#define FIN 0x4
typedef struct {
    uint32_t s_addr;  // IPv4 address for abstraction purposes
} in_addr; 
typedef struct
{
    uint32_t seq_num; // Sequence Number
    uint32_t ack_num; // Acknowledgment Number
    uint16_t flags; // Control flags (SYN, ACK, FIN)
    uint16_t window_size; // Flow control window size
} sham_header;
