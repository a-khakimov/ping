/*
 * _ping.c
 *
 *  Created on: 11 апр. 2019 г.
 *      Author: a.khakimov
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/ip_icmp.h>


#define PING_PKT_DATA_SZ    64
#define _err_ok             0
#define _err_failed         -1
#define _err_timeout        1

typedef struct {
    struct icmp hdr;
    char data[PING_PKT_DATA_SZ];
} ping_pkt_t;

typedef struct {
    struct ip ip_hdr;
    ping_pkt_t ping_pkt;
} ip_pkt_t;

static void   prepare_icmp_pkt  (ping_pkt_t *ping_pkt                                    );
static ulong  get_cur_time_ms   (                                                        );
static ushort checksum          (void *b, int len                                        );


/* _ping - Check the availability of the communication partner with a ping request.
 * result:
 * * _err_ok(0)
 * * _err_failed(-1)
 * * _err_timeout(1)
 */
int ping(const char* ip, const ulong timeout, ulong* reply_time)
{
    if (ip == NULL || timeout == 0) {
        return _err_failed;
    }

    ping_pkt_t ping_pkt;
    prepare_icmp_pkt(&ping_pkt);
    const short reply_id = ping_pkt.hdr.icmp_hun.ih_idseq.icd_id;

    struct sockaddr_in to_addr;
    to_addr.sin_family = AF_INET;
    if (!inet_aton(ip, (struct in_addr*)&to_addr.sin_addr.s_addr)) {
        printf("inet_aton\n");
        return _err_failed;
    }

    if (!strcmp(ip, "255.255.255.255") || to_addr.sin_addr.s_addr == 0xFFFFFFFF) {
        return _err_failed;
    }

    const int sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sock < 0) {
        printf("socket() %s\n", strerror(errno));
        return _err_failed;
    }

    const ulong start_time_ms = get_cur_time_ms();
    const socklen_t socklen = sizeof(struct sockaddr_in);
    if (sendto(sock, &ping_pkt, sizeof(ping_pkt_t), 0, (struct sockaddr*)&to_addr, socklen) <= 0) {
        close(sock);
        return _err_failed;
    }

    int result = _err_failed;
    struct timeval tv;
    tv.tv_sec = timeout / 1000;
    tv.tv_usec = (timeout % 1000) * 1000;

    for (;;) {
        fd_set rfd;
        FD_ZERO(&rfd);
        FD_SET(sock, &rfd);

        int n = select(sock + 1, &rfd, 0, 0, &tv);
        if (n == 0) {
            result = _err_timeout;
            break;
        }
        if (n < 0) {
            break;
        }
        const ulong elapsed_time = (get_cur_time_ms() - start_time_ms);
        if (elapsed_time > timeout) {
            result = _err_timeout;
            break;
        } else {
            const int new_timeout = timeout - elapsed_time;
            tv.tv_sec = new_timeout / 1000;
            tv.tv_usec = (new_timeout % 1000) * 1000;
        }

        if (FD_ISSET(sock, &rfd)) {
            ip_pkt_t ip_pkt;
            struct sockaddr_in from_addr;
            socklen_t socklen = sizeof(struct sockaddr_in);
            if (recvfrom(sock, &ip_pkt, sizeof(ip_pkt_t), 0, (struct sockaddr*)&from_addr, &socklen) <= 0) {
                break;
            }
            if (to_addr.sin_addr.s_addr == from_addr.sin_addr.s_addr
                    && reply_id == ip_pkt.ping_pkt.hdr.icmp_hun.ih_idseq.icd_id) {
                if (reply_time != NULL) {
                    *reply_time = elapsed_time;
                }
                result = _err_ok;
                break;
            }
        }
    }
    close(sock);
    return result;
}

static void prepare_icmp_pkt(ping_pkt_t *ping_pkt)
{
    memset(ping_pkt, 0, sizeof(ping_pkt_t));

    int i = 0;
    for (; i < sizeof(ping_pkt->data) - 1; ++i) {
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

static ulong get_cur_time_ms()
{
    struct timespec time;
    clock_gettime(CLOCK_MONOTONIC, &time);
    ulong time_ms = time.tv_sec * 1000 + (time.tv_nsec / 1000000);
    return time_ms;
}

static ushort checksum(void *b, int len)
{
    ushort *buf = b;
    uint sum=0;
    ushort result;

    for (sum = 0; len > 1; len -= 2) {
        sum += *buf++;
    }
    if (len == 1) {
        sum += *(unsigned char*)buf;
    }
    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    result = ~sum;
    return result;
}
