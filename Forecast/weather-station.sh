#! /bin/sh
### BEGIN INIT INFO
# Provides:          weather
# Required-Start:    $remote_fs $syslog $network
# Required-Stop:     $remote_fs $syslog
# Default-Start:     2 3 4 5
# Default-Stop:      0 1 6
### END INIT INFO
# If you want a command to always run, put it here
# Carry out specific functions when asked to by the system
case "$1" in
 start)
   echo "Starting weather station"
#   /sbin/ifconfig eth0 down
#   /sbin/ifconfig eth0 up 192.168.0.234
   /usr/local/bin/meteo.search.ch &
   (cd /tmp/ && python3 /usr/local/bin/weather_forecast.py) &
   ;;
 stop)
   echo "Stopping listen-for-shutdown.py"
   pkill -f /usr/local/bin/weather_forecast.py
   pkill -f /usr/local/bin/meteo.search.ch
   ;;
 *)
   echo "Usage: /etc/init.d/weather_station.sh {start|stop}"
   exit 1
   ;;
esac
exit 0
