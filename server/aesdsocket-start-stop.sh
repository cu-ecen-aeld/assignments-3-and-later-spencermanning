#!/bin/sh

case "$1" in
    start)
        echo "Starting aesdsocket"
        echo "HHHHHEEELLLOOO"
        start-stop-daemon -S -n aesdsocket -a /usr/bin/aesdsocket -- -d
        ;;
    stop)
        echo "Stopping aesdsocket"
        start-stop-daemon -K -n aesdsocket
        ;;
    *)
        echo "Usage: $0 {start|stop}"
    exit 1 # do we need the 1?
esac

exit 0