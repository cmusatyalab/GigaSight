#!/bin/sh

Guest_name=${1}
Host_port=${3}
Guest_ipaddr=${4}
Guest_port=${5}

if [ "${1}" = "${Guest_name}" ]; then
    if [ "${2}" = "stopped" -o "${2}" = "reconnect" ]; then
        iptables -t nat -D PREROUTING -p tcp --dport ${Host_port} -j DNAT \
                 --to ${Guest_ipaddr}:${Guest_port}
        iptables -D FORWARD -d ${Guest_ipaddr}/32 -p tcp -m state --state NEW \
                 -m tcp --dport ${Guest_port} -j ACCEPT
    fi
    if [ "${2}" = "start" -o "${2}" = "reconnect" ]; then
        iptables -t nat -A PREROUTING -p tcp --dport ${Host_port} -j DNAT \
                 --to ${Guest_ipaddr}:${Guest_port}
        iptables -I FORWARD -d ${Guest_ipaddr}/32 -p tcp -m state --state NEW \
                 -m tcp --dport ${Guest_port} -j ACCEPT
    fi
fi
