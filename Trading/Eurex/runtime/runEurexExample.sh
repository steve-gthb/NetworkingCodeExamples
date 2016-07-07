#!/bin/bash

source ips.env

./eurexExample -t $DFE_TOP_IP -b $DFE_BOT_IP -r $SIM_TOP_IP -n $NETMASK -p $PORT

