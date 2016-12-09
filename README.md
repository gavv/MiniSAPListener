# MiniSAPListener

MiniSAPListener is a small program that listens for SAP/SDP announcements and invokes a shell command for created, removed, or modified sessions. It can be used with [MiniSAPServer](https://wiki.videolan.org/MiniSAPServer/).

The implementation is based on PulseAudio RTP module.

Build:

    $ make

Listen announcements and print them to stdout:

    $ ./saplisten -v

Invoke shell command for every new or updated announcement:

    $ ./saplisten -c ./test_command.sh

Custom address and port:

    $ ./saplisten -v -a 224.2.127.254 -p 9875
