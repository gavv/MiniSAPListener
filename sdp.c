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

#include "sdp.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *xstrlcpy(char *b, const char *s, size_t l) {
    size_t k;

    assert(b);
    assert(s);
    assert(l > 0);

    k = strlen(s);

    if (k > l-1)
        k = l-1;

    memcpy(b, s, k);
    b[k] = 0;

    return b;
}

static bool startswith(const char *s, const char *pfx) {
    size_t l;

    assert(s);
    assert(pfx);

    l = strlen(pfx);

    return strlen(s) >= l && strncmp(s, pfx, l) == 0;
}

static bool cmp(const char* a, const char* b) {
    if (!a) {
        return !b;
    }
    if (!b) {
        return !a;
    }
    return strcmp(a, b) == 0;
}

sdp_info *sdp_parse(const char *t, int is_goodbye) {
    sdp_info *i = calloc(sizeof(sdp_info), 1);

    bool ss_valid = false;

    assert(t);
    assert(i);

    memset(i, 0, sizeof(*i));
    i->payload_type = -1;
    i->goodbye = is_goodbye;

    if (!startswith(t, SDP_HEADER)) {
        fprintf(stderr, "failed to parse SDP data: invalid header\n");
        goto fail;
    }

    t += sizeof(SDP_HEADER)-1;

    while (*t) {
        size_t l;

        l = strcspn(t, "\n");

        if (l <= 2) {
            fprintf(stderr, "failed to parse SDP data: line too short: >%s<\n", t);
            goto fail;
        }

        if (startswith(t, "o=")) {
            char a[64];
            xstrlcpy(a, t+2, l-1);

            size_t s = strcspn(a, " ");

            if (a[s]) {
                char *h;

                if ((h = strstr(a, "IN IP4 "))) {
                    char ha[64];
                    xstrlcpy(ha, h+7, 64);
                    ha[strcspn(ha, "/")] = 0;

                    i->host = strdup(ha);
                } else if ((h = strstr(a, "IN IP6 "))) {
                    char ha[64];
                    xstrlcpy(ha, h+7, 64);
                    ha[strcspn(ha, "/")] = 0;

                    i->host = strdup(ha);
                }
            }

            a[s] = 0;
            i->origin = strndup(a, s);
        } else if (startswith(t, "s=")) {
            char a[64];
            xstrlcpy(a, t+2, l-1);

            i->session = strdup(a);
        } else if (startswith(t, "c=IN IP4 ")) {
            char a[64];
            size_t k;

            k = l-8 > sizeof(a) ? sizeof(a) : l-8;

            xstrlcpy(a, t+9, k);
            a[strcspn(a, "/")] = 0;

            i->conn = strdup(a);
        } else if (startswith(t, "c=IN IP6 ")) {
            char a[64];
            size_t k;

            k = l-8 > sizeof(a) ? sizeof(a) : l-8;

            xstrlcpy(a, t+9, k);
            a[strcspn(a, "/")] = 0;

            i->conn = strdup(a);
        } else if (startswith(t, "m=audio ")) {
            if (i->payload_type == -1) {
                int port, payload;

                if (sscanf(t+8, "%i RTP/AVP %i", &port, &payload) == 2) {
                    if (port <= 0 || port > 0xFFFF) {
                        fprintf(stderr, "failed to parse SDP data: invalid port %i\n",
                                port);
                        goto fail;
                    }

                    if (payload < 0 || payload > 127) {
                        fprintf(stderr, "failed to parse SDP data: invalid payload %i\n",
                                payload);
                        goto fail;
                    }

                    i->port = port;
                    i->payload_type = payload;
                }
            }
        } else if (startswith(t, "a=rtpmap:")) {
            if (i->payload_type == -1) {
                char c[64];
                int payload;
                int len;

                if (sscanf(t + 9, "%i %n", &payload, &len) == 1) {
                    if (payload < 0 || payload > 127) {
                        fprintf(stderr, "failed to parse SDP data: invalid payload %i\n",
                                payload);
                        goto fail;
                    }
                    if (payload == i->payload_type) {
                        strncpy(c, t + 9 + len, 63);
                        c[63] = 0;
                        c[strcspn(c, "\n")] = 0;

                        i->encoding = strdup(c);
                    }
                }
            }
        }

        t += l;

        if (*t == '\n')
            t++;
    }

    return i;

fail:
    sdp_destroy(i);
    return NULL;
}

void sdp_destroy(sdp_info *i) {
    assert(i);

    free(i->origin);
    free(i->session);
    free(i->conn);
    free(i->host);
    free(i->encoding);

    free(i);
}

bool sdp_same_session(sdp_info *a, sdp_info *b) {
    if (!cmp(a->origin, b->origin))
        return false;
    if (!cmp(a->session, b->session))
        return false;
    if (!cmp(a->conn, b->conn))
        return false;
    return true;
}

bool sdp_eq(sdp_info *a, sdp_info *b) {
    if (!sdp_same_session(a, b))
        return false;
    if (!cmp(a->host, b->host))
        return false;
    if (a->port != b->port)
        return false;
    if (a->goodbye != b->goodbye)
        return false;
    if (a->payload_type != b->payload_type)
        return false;
    if (!cmp(a->encoding, b->encoding))
        return false;
    return true;
}
