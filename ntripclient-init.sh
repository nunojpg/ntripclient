#!/bin/sh
### BEGIN INIT INFO
# Provides:          ntripclient
# Required-Start:    $all
# Required-Stop:     
# Default-Start:     2 3 4 5
# Default-Stop:      0 1 6
# Short-Description: Manage NTRIP client
### END INIT INFO
    
set -e
daemon="/usr/bin/ntripclient.sh"
process_kill="ntripclient"
pidfile="/var/run/ntripclient.pid"
daemon_opts=""
    
[ -x "${daemon}" ] || exit 0

if [ -f $pidfile ]; then
   pid=$(<$pidfile)
   echo $pid
   if [ -d "/proc/$pid" ]; then
       echo "Process running"
   else
       rm -f $pidfile
       echo "Process not running, remove pidfile"
   fi
fi
    
case "${1}" in
    start)
        echo -n "Starting ntripclient: "
        if [ -s "${pidfile}" ]; then
            echo "already running, restarting"
            start-stop-daemon -K -q -p "${pidfile}"    
        fi
        start-stop-daemon -S -b -m -p "${pidfile}" -x "${daemon}" -- ${daemon_opts}
        echo "done"
        ;;
    stop)
        echo -n "Stopping ntripclient: "
        start-stop-daemon -K -q -p "${pidfile}"
        killall "${process_kill}"
        rm "${pidfile}"
        echo "done"
        ;;
    restart)
        echo -n "Restarting ntripclient: "
        start-stop-daemon -K -q -p "${pidfile}"
        killall "${process_kill}"
        rm "${pidfile}"
        start-stop-daemon -S -b -m -p "${pidfile}" -x "${daemon}" -- ${daemon_opts}
        echo "done"
    ;;
    *)
        echo "Usage: $0 {start|stop|restart}"
        exit 1
        ;;
esac

