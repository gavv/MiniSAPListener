/***
 * This file is based heavily on RTP module from PulseAudio.
 *
 * Copyright 2016 Victor Gaydov
 *
 * Licensed under GNU Lesser General Public License 2.1 or later.
 */

#include "sap.h"
#include "sdp.h"

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>

int main(int argc, char **argv)
{
    char *command = NULL;
    char *maddr = "224.2.127.254";
    int port = 9875;
    int verbose = 0;

    int c;
    while ((c = getopt (argc, argv, "vhc:a:p:")) != -1) {
        switch (c) {
        case 'c':
            command = optarg;
            break;
        case 'a':
            maddr = optarg;
            break;
        case 'p':
            port = atoi(optarg);
            if (port == 0) {
                fprintf(stderr, "invalid port %s\n", optarg);
                return 1;
            }
            break;
        case 'v':
            verbose = 1;
            break;
        case 'h':
            fprintf(stderr, "usage: %s [-v] [-a ADDRESS] [-p PORT] [-c COMMAND]\n", argv[0]);
            fprintf(stderr, "\n");
            fprintf(stderr, "OPTIONS:\n");
            fprintf(stderr, "  -a ADDRESS  multicast address to listen\n");
            fprintf(stderr, "  -p PORT     UDP port to listen\n");
            fprintf(stderr, "  -c COMMAND  shell command to invoke\n");
            fprintf(stderr, "  -v          verbose mode\n");
            return 1;
        case '?':
            if (optopt == 'c' || optopt == 'a' || optopt == 'p') {
                fprintf(stderr, "option -%c requires an argument\n", optopt);
            } else {
                fprintf(stderr, "unknown option -%c\n", optopt);
            }
            return 1;
        default:
            abort();
        }
    }

    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == -1) {
        perror("socket");
        return 1;
    }

    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("bind");
        return 1;
    }

    struct ip_mreq mreq = {};
    if (!inet_aton(maddr, &mreq.imr_multiaddr)) {
        fprintf(stderr, "failed to parse address: %s\n", maddr);
        return 1;
    }
    if (setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*)&mreq, sizeof(mreq)) == -1) {
        perror("setsockopt(IP_ADD_MEMBERSHIP)");
        return 1;
    }

    sap_context sap;
    if (!sap_init(&sap, sock)) {
        fprintf(stderr, "sap_init failed\n");
        return 1;
    }

    sdp_info **history = NULL;
    int history_size = 0;

    for (;;) {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(sock, &fds);

        int ret = select(sock + 1, &fds, NULL, NULL, NULL);
        if (ret < 0) {
            perror("select");
            return 1;
        }

        bool goodbye = false;
        if (sap_recv(&sap, &goodbye) != 0) {
            fprintf(stderr, "sap_recv returned error\n");
            return 1;
        }

        sdp_info *sdp = sdp_parse(sap.sdp_data, goodbye);
        if (!sdp) {
            fprintf(stderr, "failed to parse SDP packet\n");
            continue;
        }

        if (verbose) {
            printf("origin=%s session=%s conn=%s host=%s port=%d goodbye=%d",
                   sdp->origin,
                   sdp->session,
                   sdp->conn,
                   sdp->host,
                   sdp->port,
                   sdp->goodbye);

            if (sdp->payload_type != -1)
                printf(" pt=%d", sdp->payload_type);

            if (sdp->encoding)
                printf(" encoding=%s", sdp->encoding);

            printf("\n");
        }

        bool skip = false;
        for (size_t n = 0; n < history_size; n++) {
            if (!history[n]) {
                continue;
            }
            if (sdp_eq(history[n], sdp)) {
                skip = true;
                break;
            }
        }

        if (skip) {
            sdp_destroy(sdp);
            continue;
        }

        for (size_t n = 0; n < history_size; n++) {
            if (!history[n]) {
                continue;
            }
            if (!sdp_same_session(history[n], sdp)) {
                continue;
            }
            sdp_destroy(history[n]);
            history[n] = NULL;
        }

        if (command) {
            char cmd[1024] = {};
            int ret = snprintf(
                cmd, sizeof(cmd), "%s %s %s %s %s %d %d %d %s",
                command,
                sdp->origin,
                sdp->session,
                sdp->conn,
                sdp->host,
                sdp->port,
                sdp->goodbye,
                sdp->payload_type > 0 ? sdp->payload_type : 0,
                sdp->encoding ? sdp->encoding : "");

            if (ret > 0) {
                if (system(cmd) != 0) {
                    fprintf(stderr, "command failed: %s\n", cmd);
                }

                bool added = false;
                for (size_t n = 0; n < history_size; n++) {
                    if (history[n]) {
                        continue;
                    }
                    history[n] = sdp;
                    added = true;
                    break;
                }

                if (!added) {
                    history_size++;
                    history = realloc(history, history_size * sizeof(sdp_info*));
                    history[history_size-1] = sdp;
                }
            } else {
                fprintf(stderr, "can't format string\n");
            }
        }
    }
}
