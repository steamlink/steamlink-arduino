#!/bin/bash

# Script to create multiple bridges from SL_bridge0
#
# use SL_bridge0  to make N additional bridges 
# N.B. deletes content of SL_bridge1,2,3,...

# numebr of bridges to make i.e. SL_bridge1/ SL_bridge2
N=7



x=1
BASE=0x110
while [ $x -le $N ] ; do 
	rm SL_bridge$x/* || mkdir SL_bridge$x
	ESP_ID=`printf "0x%x" $((x * 2 + 1 + $BASE))` 
	LORA_ID=`printf "0x%x" $((x * 2 + $BASE))`
	sed -e "s/0x111/$ESP_ID/" -e "s/0x110/$LORA_ID/" -e "s/SL_bridge0/SL_bridge$x/" < SL_bridge0/SL_bridge0.ino > SL_bridge$x/SL_bridge$x.ino 
	sed -e "s/bridge273/bridge$ESP_ID/" < SL_bridge0/SL_Credentials.h > SL_bridge$x/SL_Credentials.h 

	x=$(($x + 1))
done
