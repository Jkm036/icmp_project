#include<stdio.h>
#include<string.h>
#include<unistd.h>
#include<arpa/inet.h>
#include<sys/socket.h>
#include<netinet/ip.h>
#include<netinet/ip_icmp.h>
#include<errno.h>

/* Context that the ping 
 * will be operating in
 * (Who are we pingig and what socket 
 * is this process using) 
 */
struct ping_ctx{
int sock_fd;
struct sockaddr_in dst;

}
/*Used to calculate checksum value */
unsigned short checksum(unsigned short* buf, int len){

	unsigned long sum =  0;
	/* Iterating throughpacket 
	 * and adding memory 
	 * as if an array of 
	 * unsigned shorts
	 */
	while(len >1){
		sum += *(buff++);
		len -=2;
	}
	/* If there is one more byte left over,
	 * we should include add it to the sum
	 */
	if(len ==1)
		sum +=  *((unsigned char *) buf)l
	/* We are we shifting right 2 bytes 
	 * and adding that to the sum shifted right
	 * 2 bytes again?
	 */
	sum = (sum >> 16) + (sum & 0xFFFF);
	sum += (sum >> 16);
	 
	/* This either should be all 1's or shoul dbe checked 
	 * against the checksum to be all 1's
	 */	
	return ~sum;
}
/*Initiatize ping context*/
struct ping_ctx init_ping(){
	/*Create object to represent the ping context*/
	struct ping_ctx* ctx = malloc(sizeof(struct ping_ctx));

	/*Error checking*/
	if(!ctx){
		perror("Could not allocate memory to store ping context...");
		free(ctx);
		return NULL;
	}
	ctx
	return ctx;

}
int main(){
}

