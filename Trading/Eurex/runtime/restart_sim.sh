#!/bin/bash

source ips.env

export INIT_TOP=QSFP_TOP_10G_PORT1:$SIM_TOP_IP:$NETMASK
export INIT_BOT=QSFP_BOT_10G_PORT1:$SIM_BOT_IP:$NETMASK

maxcompilersim -n sim -c ISCA -e $INIT_TOP -e $INIT_BOT -p QSFP_TOP_10G_PORT1:top1.pcap  restart


