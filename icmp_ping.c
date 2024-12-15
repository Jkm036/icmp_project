#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<arpa/inet.h>
#include<sys/socket.h>
#include<netinet/ip.h>
#include<netinet/ip_icmp.h>
#include<errno.h>
#include<pthread.h>
#include<signal.h>
#include<stdbool.h>
//MACROS for ease of use
#define PACKET_SIZE 64 //Why 64? is this the  maximum size of an ICMP echo request
#define ICMP_HDR_LEN 8

/*
 * Note:
 * inet_ntoa() is:
 * 1. Older than inet_ntop()
 * 2. Not thread safe
 */

 /* Context that the ping 
 * will be operating in
 * (Who are we pingig and what socket 
 * is this process using) 
 */
struct ping_ctx{
    int sock_fd;
    struct sockaddr_in dst;
    //Number of pings to send
    int count;
    volatile int running;
    pthread_t send_thread;
    pthread_t recv_thread;
};

/*Function declarations*/
static int check_valid_reply(struct icmp*);
unsigned short checksum(unsigned short* buff, int len);
struct ping_ctx* init_ping(const char * dst,int count);
int send_ping( struct ping_ctx* ctx);
int receive_ping(struct ping_ctx* ctx);
void cleanup_ping(struct ping_ctx* ctx);
void* receive_ping_thread(void *arg);
void* send_ping_thread(void*arg);

/*Used to calculate checksum value */
unsigned short checksum(unsigned short* buff, int len){

	uint32_t sum =  0;
	/* Iterating throughpacket 
	 * and adding memory 
	 * as if an array of 
	 * unsigned shorts
	 */
	while(len>1){
		sum += *(buff++);
		len -=2;
	}
	/* If there is one more byte left over,
	 * we should include add it to the sum
	 */
	if(len ==1)
		sum +=  *((unsigned char *) buff);
	 
	/*Folding: also called One's complement addittion*/
	sum = (sum >> 16) + (sum & 0xFFFF);
	sum += (sum >> 16);
	 
	/* This either should be all 1's or shoul dbe checked 
	 * against the checksum to be all 1's
	 */	
	return ~sum;
}
/*Initiatize ping context for the ping context*/
struct ping_ctx* init_ping(const char * dst, int count){
	/*Create object to represent the ping context*/
	struct ping_ctx* ctx = calloc(1, sizeof(struct ping_ctx));


	/*Error checking*/
	if(!ctx){
		perror("Could not allocate memory to store ping context...");
		free(ctx);
		return NULL;
	}
	/*Any incoming packet with an ICMP
	 * protocol identifier in the packet header will be sent here*/
	ctx->sock_fd = socket(AF_INET,
			SOCK_RAW,
			IPPROTO_ICMP);
	if(ctx-> sock_fd < 0){
		perror("Socket creation failed");
		free(ctx);
		ctx=NULL;
		return NULL;
	}
	/*Setting a timeout for the socket*/
	struct timeval tv;	
	tv.tv_sec = 1;
	tv.tv_usec = 0;
	/*We want to set socket at the most basic level
	 *
	 * What levels exist for socket options:
	 * 1. IPPROTO_IP
	 * 2. IPPROTO_TCP
	 * 3. SOL_SOCKET is for options that are protocol independent
	 * 
	 * SO_RCVTIMEO specifies SO_RCVTIMEO option (timeout on listening to socket)
	 * If not dat arrives in this time... the call returns EAGAIN or EWOULDBLOCk error
	 */	
	if(setsockopt(ctx->sock_fd, 
		      SOL_SOCKET, 
		      SO_RCVTIMEO, 
		      &tv, 
		      sizeof(tv)) <0)
	{
		perror("Failed to set socket timeout");
		close(ctx->sock_fd);
		free(ctx);
		ctx=NULL;
		return NULL;
	};

	/*context address informatoin*/
	ctx->dst.sin_family = AF_INET;
	ctx->dst.sin_addr.s_addr = inet_addr(dst) ;
	/*Ping context informattion*/
	ctx->count = count;
	ctx->running = 1;

	return ctx;
}

/* Function responsible for sending the ping 
 * using a created ping context
 */
int send_ping( struct ping_ctx* ctx){
	char packet[ICMP_HDR_LEN];	
	/*Pointer to structure that represents ICMP packet*/
	struct icmp*  icmp = (struct icmp*)packet;
	
	/*Clearing packet information before use*/
	memset(packet, 0, ICMP_HDR_LEN);
	 
	/*Fill ICMP header*/
	icmp->icmp_type = ICMP_ECHO;
	icmp->icmp_code = 0;
	/*Used in absence of port*/
	icmp->icmp_id   = getpid();
	icmp->icmp_seq  = 0;
	
	/*Calculate checksum and append to ICMP packet*/
	icmp->icmp_cksum = 0;
	icmp->icmp_cksum = checksum((unsigned short* )packet, ICMP_HDR_LEN);
	/*sendto function signature:
	 * file descriptor
	 * pointer to packet
	 * size of the packet
	 * flags
	 * struct sockaddr*
	 * size of the sin_addr??? I guess
	 */ 
	if(sendto(ctx->sock_fd, 
		  packet, 
		  ICMP_HDR_LEN, 
		  0,
		  (struct sockaddr*)&ctx->dst,
		  sizeof(ctx->dst))<0){
		perror("Failed to send packet out of socket");
		return -1;
	}
	printf("Sent ICMP echo request to %s\n", inet_ntoa(ctx->dst.sin_addr));
	return 0;
}

int receive_ping(struct ping_ctx* ctx){
	char buffer[PACKET_SIZE];
	struct ip* ip_header;
	struct sockaddr_in sender;
	socklen_t sender_len = sizeof(sender); 

	/*Here we are receiving the full IP packet header!*/
	if(recvfrom(ctx->sock_fd,
		    buffer,
		    PACKET_SIZE,
		    0,
		    (struct sockaddr*)&sender,
		    &sender_len)<=0){
		/*If we times outm we don't need to Log an error message*/
		if(!errno == EAGAIN && !errno == EWOULDBLOCK )
			perror("Failed to receive");
		return -1;

	}
	/*Discard IP header ... Wait why tf am I even doing this?*/
	ip_header = (struct ip*)buffer;
		//Common knowledge that IP header lengths are stored in units 
		//of 4 bytes... (Remember that)
	int ip_header_len = (ip_header->ip_hl * 4);
	struct icmp* icmp = (struct icmp*) (buffer + ip_header_len);
	/*Error checking on ICMP packet*/
	if(icmp->icmp_type == ICMP_ECHO){
		printf("Caught my own request\n");
		return 0;
	}
	if(check_valid_reply(icmp)<0) return -1;
	/*Error checking on reported destination*/
	if (sender.sin_addr.s_addr != ctx->dst.sin_addr.s_addr){
		 printf("Mismatching Remote addresses" 
			"between ping context and IP packet");
	}
	/* Length of ascribed address in bits*/
	printf("The sender length here is %u\n", sender_len);
	/*Succesfully receiving a packet*/
	printf("Received packet from %s\n", inet_ntoa(sender.sin_addr));
	return 0;
}
static int check_valid_reply(struct icmp* reply){
	bool error =false;

	if (reply->icmp_type != ICMP_ECHOREPLY){
		printf("Recieved wrong ICMP type\n"
		       "Should be (%d) but got (%d)\n", ICMP_ECHOREPLY, reply->icmp_type); 
		error = true;
	}
	if (reply->icmp_id != getpid()){
		printf("Received wrong ICMP id\n"
		       "Should be (%d) but got (%d)\n", getpid(), reply->icmp_id); 
		error = true;
	}	
	/*Making sure checksum is valid*/
	unsigned short cksum = checksum((unsigned short*)reply, ICMP_HDR_LEN);
	if (cksum){
		printf("Checksum failed\n"); 
		error = true;
	}
	/*Did we find an error?*/
	if(error){
		printf("Please review aforementioned errors\n");
		return -1;
	}
	return 0;
}
/*Cleanup command for */
void cleanup_ping(struct ping_ctx* ctx){
	if(ctx){
		ctx->running=0;
		// Stop threads 
		pthread_join(ctx->send_thread, NULL);
		pthread_join(ctx->recv_thread, NULL);
		close(ctx->sock_fd);
		free(ctx);
		ctx = NULL;
	}
}
//Handle clearnup on Ctrl+c
void signal_handler(int signo){
	exit(0);
}

/*Multithreaded functions*/
//Thread responsible for sending pings
void* send_ping_thread(void*arg){
	struct ping_ctx* ctx = (struct ping_ctx*)arg;
	int sent = 0;
	/* Keep sending until we 
	 * reached the amount of 
	 * passed in pings
	 */

	while(ctx->running && ( (ctx->count ==-1) || (sent < ctx->count))){
		if(send_ping(ctx) <0 ) break;
		sent++;
		sleep(1);
	}
	/*Once we are done sending pings...*/
	if(ctx->count !=-1 && sent >= ctx->count){
		ctx->running =0;
		/* Make sure recv thread isn't hanging 
		 * still listening to thsi socket
		 */
		close(ctx->sock_fd);
	}
	return NULL;
}

/*Seperate thread than sender thread*/
void* receive_ping_thread(void *arg){

	/*Expecting a Ping context to listen to socket*/
	struct ping_ctx* ctx = (struct ping_ctx*) arg;
	/*Set timeout for listening to socket*/
	
	
	while(ctx->running){
		if(receive_ping(ctx) < 0){
			if (errno == EAGAIN || errno == EWOULDBLOCK){
				 // This means a tiemout occured
				continue;
			}
			//Real error occured
			break;
		}
	}
	return NULL;
}

/*Here we should just send the ping
 * I guess we have to continuously run
 * becuase we need to check for recieved 
 * pings.... I don't know yet we'll see
 */
int main(int argc , char* argv[]){
	int count = -1; //default to infinite pings
	char * dest_ip= NULL;
	int opt;

	/*Pull options from command line call*/
	while((opt = getopt(argc, argv, "c:"))!=-1){
		switch(opt){
		case 'c':
			count = atoi(optarg);
			break;
		default:
			printf("Usage: %s [-c count] destination\n", argv[0]);
			return 1;
		}
	}
	/* optind I guess keeps track of 
	 * the pointer in the 
	 * list of strings passed 
	 * into the program
	 */
	if(optind >=argc){
		printf("Expected destination IP address");
		printf("Usage: %s [-c count] destination\n", argv[0]);
		return 1;
	}
	
	dest_ip = argv[optind];
	optind++;

	// Setting handler for signal
	signal(SIGINT, signal_handler);

	/*Creating context of this ping command*/
	struct ping_ctx* ctx = init_ping(dest_ip, count);
	if(!ctx){
		perror("Something when wrong with creating the ping context");
		return 1;
	}
	/*Creating Threads*/
	pthread_create(&ctx->send_thread, NULL, send_ping_thread, ctx);
	pthread_create(&ctx->recv_thread, NULL, receive_ping_thread, ctx);

	// Wait for threads
	pthread_join(ctx->send_thread,NULL);
	pthread_join(ctx->recv_thread,NULL);
	
	cleanup_ping(ctx);
	return 0;
}


