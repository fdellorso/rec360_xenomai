#!/bin/bash
cat=0
cmd=0
dat=0
mask=0xFF

echo -e "StuFA Command Tester [q to quit]"

while [ 1 ]
do
    echo -e "\nCommand category: "; read cat
    if [[ $cat = [qQ] ]]; then
        break
    elif [[($cat -lt 0) || ($cat -gt 15)]]; then 
        continue
    fi

    echo -e "Command operation: "; read cmd
    if [[ $cmd = [qQ] ]]; then
        break
    elif [[($cmd -lt 0) || ($cmd -gt 15)]]; then 
        continue
    fi

    echo -e "Command data: "; read dat
    if [[ $dat = [qQ] ]]; then
        break
    fi

    printf -v hcat    "%x" "$(( (cat >> 0 ) & 0x0F ))"
    printf -v hcmd    "%x" "$(( (cmd >> 0 ) & 0x0F ))"
    printf -v hdat1 "%02x" "$(( (dat >> 16) & 0xFF ))"
    printf -v hdat2 "%02x" "$(( (dat >> 8 ) & 0xFF ))"
    printf -v hdat3 "%02x" "$(( (dat >> 0 ) & 0xFF ))"

    echo -n -e \\x$hcat$hcmd\\x$hdat1\\x$hdat2\\x$hdat3 > /dev/ble_cdev_rx

    # printf -v hcat "%x" "$cat"
    # printf -v hcmd "%x" "$cmd"
    # printf -v hdat "%x" "$dat"
    # echo -n -e \\x$cat$cmd
    # echo -n -e \\x$(( (dat >> 8) & 0xFF ))
    # echo -n -e \\x$(( (hdat >> 8) & 0xFF ))
    # echo -n -e $hcat$hcmd > /dev/ble_cdev_rx
done