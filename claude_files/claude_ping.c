#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<arpa/inet.h>
#include<sys/socket.h>
#include<netinet/ip.h>
#include<netinet/ip_icmp.h>
#include<errno.h>

#define PACKET_SIZE 64
#define ICMP_HEADER_LEN 8

struct ping_ctx {
    int sock_fd;
    struct sockaddr_in dst;
};

unsigned short checksum(unsigned short* buff, int len) {
    unsigned long sum = 0;
    while(len > 1) {
        sum += *(buff++);
        len -= 2;
    }
    if(len == 1)
        sum += *((unsigned char*)buff);
    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    return ~sum;
}

struct ping_ctx* init_ping(const char* dest_ip) {
    struct ping_ctx* ctx = malloc(sizeof(struct ping_ctx));
    if(!ctx) {
        perror("Could not allocate memory to store ping context...");
        return NULL;
    }

    // Create raw socket
    ctx->sock_fd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if(ctx->sock_fd < 0) {
        perror("Socket creation failed");
        free(ctx);
        return NULL;
    }

    // Set destination address
    ctx->dst.sin_family = AF_INET;
    ctx->dst.sin_addr.s_addr = inet_addr(dest_ip);

    return ctx;
}

int send_ping(struct ping_ctx* ctx) {
    char packet[PACKET_SIZE];
    struct icmp* icmp = (struct icmp*)packet;
    
    // Fill ICMP header
    memset(packet, 0, PACKET_SIZE);
    icmp->icmp_type = ICMP_ECHO;
    icmp->icmp_code = 0;
    icmp->icmp_id = getpid();
    icmp->icmp_seq = 0;
    
    // Calculate checksum
    icmp->icmp_cksum = 0;
    icmp->icmp_cksum = checksum((unsigned short*)icmp, PACKET_SIZE);

    // Send packet
    if(sendto(ctx->sock_fd, packet, PACKET_SIZE, 0, 
              (struct sockaddr*)&ctx->dst, sizeof(ctx->dst)) <= 0) {
        perror("Failed to send packet");
        return -1;
    }
    
    printf("Sent ICMP echo request to %s\n", inet_ntoa(ctx->dst.sin_addr));
    return 0;
}

int receive_ping(struct ping_ctx* ctx) {
    char buffer[PACKET_SIZE];
    struct sockaddr_in sender;
    socklen_t sender_len = sizeof(sender);

    // Receive packet
    if(recvfrom(ctx->sock_fd, buffer, PACKET_SIZE, 0,
                (struct sockaddr*)&sender, &sender_len) <= 0) {
        perror("Failed to receive packet");
        return -1;
    }

    printf("Received ICMP echo reply from %s\n", inet_ntoa(sender.sin_addr));
    return 0;
}

void cleanup_ping(struct ping_ctx* ctx) {
    if(ctx) {
        close(ctx->sock_fd);
        free(ctx);
    }
}

int main(int argc, char* argv[]) {
    if(argc != 2) {
        printf("Usage: %s <destination IP>\n", argv[0]);
        return 1;
    }

    struct ping_ctx* ctx = init_ping(argv[1]);
    if(!ctx) return 1;

    while(1) {
        if(send_ping(ctx) < 0) break;
        if(receive_ping(ctx) < 0) break;
        sleep(1);
    }

    cleanup_ping(ctx);
    return 0;
}
