# MiniSAPListener

MiniSAPListener is a small program that listens for SAP/SDP announcements and invokes a shell command for created, removed, or modified sessions. It can be used with [MiniSAPServer](https://wiki.videolan.org/MiniSAPServer/).

See usage example in [this blog post](https://enise.org/gavv/blog/minisaplistener/).

The implementation is based on [PulseAudio](https://www.freedesktop.org/wiki/Software/PulseAudio/) RTP module.

Build:

    $ make

Listen announcements and print them to stdout:

    $ ./saplisten -v

Invoke a shell command for every created, modified, or removed session:

    $ ./saplisten -c ./test_command.sh

Use custom address and port:

    $ ./saplisten -v -a 224.2.127.254 -p 9875
