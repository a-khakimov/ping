/*
 * _ping.c
 *
 *  Created on: 11 апр. 2019 г.
 *      Author: a.khakimov
 */

#include "proto.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip_icmp.h>

unsigned short checksum(void *b, int len);

#define PING_PKT_SZ 	64

typedef struct {
	struct icmp hdr;
	char data[PING_PKT_SZ - sizeof(struct icmp)];
} ping_pkt_t;

typedef struct {
	struct ip ip_hdr;
	ping_pkt_t ping_pkt;
} ip_pkt_t;

/* _ping - Check the availability of the communication partner with a ping request.
 * ip - IP address of the communication partner as string
 * timeout - Timeout in milliseconds to wait until reply
 * reply_time - Pointer to get average reply time of the ping request in milliseconds
 */

int _ping(devctl_ping_t* p)
{
	int sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);

	struct sockaddr_in addr_con;
	addr_con.sin_family = AF_INET;
	inet_aton(p->args.ip, (struct in_addr*) &addr_con.sin_addr.s_addr);
	socklen_t socklen = sizeof(addr_con);

	ping_pkt_t ping_pkt;
	memset(&ping_pkt, 0, sizeof(ping_pkt));

	int i = 0;
	for(; i < sizeof(ping_pkt.data) - 1; ++i) {
		ping_pkt.data[i] = i + 'a';
	}
	ping_pkt.data[i] = 0;

	srand(time(NULL));
	const short random_id = rand();
	ping_pkt.hdr.icmp_type = ICMP_ECHO;
	ping_pkt.hdr.icmp_hun.ih_idseq.icd_id = random_id;
	ping_pkt.hdr.icmp_hun.ih_idseq.icd_seq = 0;
	ping_pkt.hdr.icmp_cksum = checksum(&ping_pkt, sizeof(ping_pkt));

	struct timespec time_start, time_end;
	clock_gettime(CLOCK_MONOTONIC, &time_start);

	unsigned long wait_time_ms = p->args.timeout;
	struct timespec wait_time;
	wait_time.tv_sec = wait_time_ms / 1000;
	wait_time.tv_nsec = wait_time_ms * 1000000;
	if(setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &wait_time, sizeof(wait_time))) {
		perror("Setting socket wait time failed \n");
		p->result.return_ = 1;
		return sizeof(p->result);
	}

	if(sendto(sock, &ping_pkt, PING_PKT_SZ, 0, (struct sockaddr*)&addr_con, socklen) <= 0) {
		printf("Send echo failed \n");
		p->result.return_ = 1;
		return sizeof(p->result);
	}

	ip_pkt_t ip_pkt;
	struct sockaddr_in r_addr;
	if(recvfrom(sock,  &ip_pkt, sizeof(ip_pkt), 0, (struct sockaddr*)&r_addr, &socklen) <= 0) {
		perror("Get reply failed \n");
		p->result.return_ = 1;
		return sizeof(p->result);
	}

	clock_gettime(CLOCK_MONOTONIC, &time_end);

	if(ip_pkt.ping_pkt.hdr.icmp_hun.ih_idseq.icd_id != random_id) {
		perror("Reply and request IDs is not equal \n");
		p->result.return_ = 1;
		return sizeof(p->result);
	}

	const unsigned long start_time_ms = time_start.tv_sec * 1000 + (time_start.tv_nsec / 1000000);
	const unsigned long end_time_ms = time_end.tv_sec * 1000 + (time_end.tv_nsec / 1000000);
	p->result.reply_time = end_time_ms - start_time_ms;
	p->result.return_ = 0;

	return sizeof(p->result);
}

unsigned short checksum(void *b, int len)
{
	unsigned short *buf = b;
	unsigned int sum=0;
	unsigned short result;

	for ( sum = 0; len > 1; len -= 2 )
		sum += *buf++;
	if ( len == 1 )
		sum += *(unsigned char*)buf;
	sum = (sum >> 16) + (sum & 0xFFFF);
	sum += (sum >> 16);
	result = ~sum;
	return result;
}


