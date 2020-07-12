#!/bin/bash

#Jetson Tx2 Carrier board J21 P36.(UART#0 Clear to Send)

echo "467" | sudo tee /sys/class/gpio/export
echo "out" | sudo tee /sys/class/gpio/gpio467/direction
while true
do
echo date "high"
echo "1" | sudo tee /sys/class/gpio/gpio467/value
sleep 0.2

echo date "low"
echo "0" | sudo tee /sys/class/gpio/gpio467/value
sleep 0.2
done
