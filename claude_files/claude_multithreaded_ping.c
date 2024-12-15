#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>

#define PACKET_SIZE 64
#define ICMP_HDR_LEN 8

struct ping_ctx {
    int sock_fd;
    struct sockaddr_in dst;
    int count;          // Number of pings to send
    volatile int running; // Thread control
    pthread_t send_thread;
    pthread_t recv_thread;
};

// Rest of your existing code (checksum function) here...

void* send_ping_thread(void* arg) {
    struct ping_ctx* ctx = (struct ping_ctx*)arg;
    int sent = 0;

    while(ctx->running && (ctx->count == -1 || sent < ctx->count)) {
        if(send_ping(ctx) < 0) break;
        sent++;
        sleep(1);
    }

    if(ctx->count != -1 && sent >= ctx->count) {
        ctx->running = 0;
    }
    return NULL;
}

void* receive_ping_thread(void* arg) {
    struct ping_ctx* ctx = (struct ping_ctx*)arg;
    
    while(ctx->running) {
        if(receive_ping(ctx) < 0) break;
    }
    return NULL;
}

struct ping_ctx* init_ping(const char* dst, int count) {
    struct ping_ctx* ctx = calloc(1, sizeof(struct ping_ctx));
    if(!ctx) {
        perror("Could not allocate memory");
        return NULL;
    }

    ctx->sock_fd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if(ctx->sock_fd < 0) {
        perror("Socket creation failed");
        free(ctx);
        return NULL;
    }

    ctx->dst.sin_family = AF_INET;
    ctx->dst.sin_addr.s_addr = inet_addr(dst);
    ctx->count = count;
    ctx->running = 1;

    return ctx;
}

void cleanup_ping(struct ping_ctx* ctx) {
    if(ctx) {
        ctx->running = 0;
        pthread_join(ctx->send_thread, NULL);
        pthread_join(ctx->recv_thread, NULL);
        close(ctx->sock_fd);
        free(ctx);
    }
}

void signal_handler(int signo) {
    // Handle cleanup on ctrl+c
    exit(0);
}

int main(int argc, char* argv[]) {
    int count = -1;  // -1 means infinite
    char* dest_ip = NULL;
    int opt;

    while ((opt = getopt(argc, argv, "c:")) != -1) {
        switch (opt) {
            case 'c':
                count = atoi(optarg);
                break;
            default:
                printf("Usage: %s [-c count] destination\n", argv[0]);
                return 1;
        }
    }

    if (optind >= argc) {
        printf("Expected destination IP address\n");
        return 1;
    }
    dest_ip = argv[optind];

    signal(SIGINT, signal_handler);

    struct ping_ctx* ctx = init_ping(dest_ip, count);
    if(!ctx) return 1;

    // Create threads
    pthread_create(&ctx->send_thread, NULL, send_ping_thread, ctx);
    pthread_create(&ctx->recv_thread, NULL, receive_ping_thread, ctx);

    // Wait for threads
    pthread_join(ctx->send_thread, NULL);
    pthread_join(ctx->recv_thread, NULL);

    cleanup_ping(ctx);
    
