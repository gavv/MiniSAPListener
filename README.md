# MiniSAPListener

[![Build](https://github.com/gavv/MiniSAPListener/workflows/build/badge.svg)](https://github.com/gavv/MiniSAPListener/actions)

MiniSAPListener is a small program that listens to SAP/SDP announcements and invokes a shell command for created, removed, or modified sessions. It can be used with [MiniSAPServer](https://wiki.videolan.org/MiniSAPServer/).

See usage example in [this article](https://gavv.github.io/articles/minisaplistener/).

The implementation is based on the [PulseAudio](https://www.freedesktop.org/wiki/Software/PulseAudio/) RTP module.

Build:

    make

Install system-wide:

    make install

Listen to announcements and print them to stdout:

    ./saplisten -v

Invoke a shell command for every created, modified, or removed session:

    ./saplisten -c ./test_command.sh

Use custom address and port:

    ./saplisten -v -a 224.2.127.254 -p 9875
