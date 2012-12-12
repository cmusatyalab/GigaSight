#!/bin/bash
GUEST_IP=`sudo ./virt-addr.py private-vm2`
echo "port forwarding for ${GUEST_IP}"

sudo ./portforwarding.sh personal_vm stopped 2229 ${GUEST_IP} 22 >/dev/null 2>&1
sudo ./portforwarding.sh personal_vm start 2229 ${GUEST_IP} 22

sudo ./portforwarding.sh personal_vm stopped 12346 ${GUEST_IP} 12345 >/dev/null 2>&1
sudo ./portforwarding.sh personal_vm start 12346 ${GUEST_IP} 12345

sudo ./portforwarding.sh personal_vm stopped 5556 ${GUEST_IP} 5555 >/dev/null 2>&1
sudo ./portforwarding.sh personal_vm start 5556 ${GUEST_IP} 5555 
