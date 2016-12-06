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

#ifndef SDP_H_
#define SDP_H_

#include <stdbool.h>

#define SDP_HEADER "v=0\n"

typedef struct sdp_info {
    char *origin;
    char *session;
    char *conn;
    char *host;
    int port;
    int goodbye;
    int payload_type;
    char *encoding;
} sdp_info;

sdp_info *sdp_parse(const char *t, int is_goodbye);
void sdp_destroy(sdp_info *i);

bool sdp_same_session(sdp_info *a, sdp_info *b);
bool sdp_eq(sdp_info *a, sdp_info *b);

#endif
