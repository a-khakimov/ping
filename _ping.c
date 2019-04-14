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
#include <stdbool.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip_icmp.h>


#define PING_PKT_DATA_SZ 	32

typedef struct {
	struct icmp hdr;
	char data[PING_PKT_DATA_SZ];
} ping_pkt_t;

typedef struct {
	struct ip ip_hdr;
	ping_pkt_t ping_pkt;
} ip_pkt_t;

bool set_socket_rcv_timeout(int sock, const unsigned long timeout);
bool receive_ping_pkt(int sock, ip_pkt_t* ip_pkt, const unsigned long timeout);
bool send_ping_pkt(int sock, const char* ip, ping_pkt_t* ping_pkt);

void prepare_icmp_pkt(ping_pkt_t *ping_pkt);
unsigned long get_cur_time_ms();
unsigned short checksum(void *b, int len);

/* _ping - Check the availability of the communication partner with a ping request.
 * ip - IP address of the communication partner as string
 * timeout - Timeout in milliseconds to wait until reply
 * reply_time - Pointer to get average reply time of the ping request in milliseconds
 */

int _ping(devctl_ping_t* p)
{
	char ip[IPSTR_MAXLEN];
	strcpy(ip, p->args.ip);
	unsigned long timeout = p->args.timeout;
	p->result.return_ = 1;

	int sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	if(sock == -1) {
		p->result.return_ = 1;
		return sizeof(p->result);
	}

	ping_pkt_t ping_pkt;
	prepare_icmp_pkt(&ping_pkt);
	const short reply_id = ping_pkt.hdr.icmp_hun.ih_idseq.icd_id;
	const unsigned long start_time_ms = get_cur_time_ms();

	if(send_ping_pkt(sock, ip, &ping_pkt)) {
		ip_pkt_t ip_pkt;
		if(receive_ping_pkt(sock, &ip_pkt, timeout)) {
			const unsigned long end_time_ms = get_cur_time_ms();
			const short request_id = ip_pkt.ping_pkt.hdr.icmp_hun.ih_idseq.icd_id;
			if(request_id == reply_id) {
				p->result.reply_time = end_time_ms - start_time_ms;
				p->result.return_ = 0;
			}
		}
	}

	close(sock);
	return sizeof(p->result);
}


bool set_socket_rcv_timeout(int sock, const unsigned long timeout)
{
	bool result = true;
	unsigned long wait_time_ms = timeout;
	struct timespec wait_time;
	wait_time.tv_sec = wait_time_ms / 1000;
	wait_time.tv_nsec = wait_time_ms * 1000000;
	if(setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &wait_time, sizeof(wait_time))) {
		result = false;
	}
	return result;
}

bool receive_ping_pkt(int sock, ip_pkt_t* ip_pkt, const unsigned long timeout)
{
	bool result = true;
	if(!set_socket_rcv_timeout(sock, timeout)) {
		return (result = false);
	}

	struct sockaddr_in r_addr;
	socklen_t socklen = sizeof(struct sockaddr_in);
	if(recvfrom(sock,  ip_pkt, sizeof(ip_pkt_t), 0, (struct sockaddr*)&r_addr, &socklen) <= 0) {
		result = false;
	}
	return result;
}

bool send_ping_pkt(int sock, const char* ip, ping_pkt_t* ping_pkt)
{
	bool result = true;
	struct sockaddr_in addr_con;
	addr_con.sin_family = AF_INET;
	inet_aton(ip, (struct in_addr*) &addr_con.sin_addr.s_addr);
	socklen_t socklen = sizeof(struct sockaddr_in);

	if(sendto(sock, ping_pkt, sizeof(ping_pkt_t), 0, (struct sockaddr*)&addr_con, socklen) <= 0) {
		result = false;
	}
	return result;
}

void prepare_icmp_pkt(ping_pkt_t *ping_pkt)
{
	memset(ping_pkt, 0, sizeof(ping_pkt_t));

	int i = 0;
	for(; i < sizeof(ping_pkt->data) - 1; ++i) {
		ping_pkt->data[i] = i + 'a';
	}
	ping_pkt->data[i] = 0;

	srand(time(NULL));
	const short random_id = rand();
	ping_pkt->hdr.icmp_type = ICMP_ECHO;
	ping_pkt->hdr.icmp_hun.ih_idseq.icd_id = random_id;
	ping_pkt->hdr.icmp_hun.ih_idseq.icd_seq = 0;
	ping_pkt->hdr.icmp_cksum = checksum(ping_pkt, sizeof(ping_pkt_t));
}

unsigned long get_cur_time_ms()
{
	struct timespec time;
	clock_gettime(CLOCK_MONOTONIC, &time);
	unsigned long time_ms = time.tv_sec * 1000 + (time.tv_nsec / 1000000);
	return time_ms;
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


