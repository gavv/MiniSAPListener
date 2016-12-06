/***
 * This file is based heavily on RTP module from PulseAudio.
 *
 * Copyright 2016 Victor Gaydov
 */

/***
  This file is part of PulseAudio.

  Copyright 2006 Lennart Poettering

  PulseAudio is free software; you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License as published
  by the Free Software Foundation; either version 2.1 of the License,
  or (at your option) any later version.

  PulseAudio is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with PulseAudio; if not, see <http://www.gnu.org/licenses/>.
***/

#include "sap.h"
#include "sdp.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>

#define MIME_TYPE "application/sdp"

sap_context* sap_init(sap_context *c, int fd) {
    assert(c);
    assert(fd >= 0);

    c->fd = fd;
    c->sdp_data = NULL;
    return c;
}

void sap_destroy(sap_context *c) {
    assert(c);

    if (c->fd != -1) {
        close(c->fd);
    }

    free(c->sdp_data);

    c->fd = -1;
    c->sdp_data = NULL;
}

int sap_recv(sap_context *c, bool *goodbye) {
    struct msghdr m;
    struct iovec iov;
    int size;
    char *buf = NULL, *e;
    uint32_t header;
    unsigned six, ac, k;
    ssize_t r;

    assert(c);
    assert(goodbye);

    if (ioctl(c->fd, FIONREAD, &size) < 0) {
        fprintf(stderr, "ioctl(FIONREAD) failed: %s\n", strerror(errno));
        goto fail;
    }

    buf = calloc(sizeof(char), (unsigned)size+1);
    buf[size] = 0;

    iov.iov_base = buf;
    iov.iov_len = (size_t) size;

    m.msg_name = NULL;
    m.msg_namelen = 0;
    m.msg_iov = &iov;
    m.msg_iovlen = 1;
    m.msg_control = NULL;
    m.msg_controllen = 0;
    m.msg_flags = 0;

    if ((r = recvmsg(c->fd, &m, 0)) != size) {
        fprintf(stderr, "recvmsg() failed: %s\n",
                r < 0 ? strerror(errno) : "size mismatch");
        goto fail;
    }

    if (size < 4) {
        fprintf(stderr, "SAP packet too short\n");
        goto fail;
    }

    memcpy(&header, buf, sizeof(uint32_t));
    header = ntohl(header);

    if (header >> 29 != 1) {
        fprintf(stderr, "unsupported SAP version\n");
        goto fail;
    }

    if ((header >> 25) & 1) {
        fprintf(stderr, "encrypted SAP not supported\n");
        goto fail;
    }

    if ((header >> 24) & 1) {
        fprintf(stderr, "compressed SAP not supported\n");
        goto fail;
    }

    six = (header >> 28) & 1U;
    ac = (header >> 16) & 0xFFU;

    k = 4 + (six ? 16U : 4U) + ac*4U;
    if ((unsigned) size < k) {
        fprintf(stderr, "SAP packet too short (AD)\n");
        goto fail;
    }

    e = buf + k;
    size -= (int) k;

    if ((unsigned) size >= sizeof(MIME_TYPE) && !strcmp(e, MIME_TYPE)) {
        e += sizeof(MIME_TYPE);
        size -= (int) sizeof(MIME_TYPE);
    } else if ((unsigned) size < sizeof(SDP_HEADER)-1
               || strncmp(e, SDP_HEADER, sizeof(SDP_HEADER)-1)) {
        fprintf(stderr, "invalid SDP header\n");
        goto fail;
    }

    if (c->sdp_data)
        free(c->sdp_data);

    c->sdp_data = strndup(e, (unsigned)size);
    free(buf);

    *goodbye = !!((header >> 26) & 1);
    return 0;

fail:
    free(buf);
    return -1;
}
