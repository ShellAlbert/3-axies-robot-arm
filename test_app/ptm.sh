#!/bin/bash

#TechServo RJS17 Profile Torque Mode.
#2020/9/3,zhangshaoyan +8613522296239.

#clear fault.
#bit7=1.
#Reset Fault.A low-to-high transition of this bit makes the amplifier attempt to clear any latched fault condition.
ethercat download -p 0 -t uint16 0x6040 0x00 0x80

#Sync Manager2,Synchronization Type=0:Free Run.
ethercat download -p 0 -t uint16 0x1c32 0x01 0
ethercat states -p 0 OP

#Mode of Operation=4,Profile Torque mode.
ethercat download -p 0 -t uint8 0x6060 0x00 4

#Torque Slope (Torque acceleration or deceleration).
#set to zero to disable slope limiting for instant response.
ethercat download -p 0 -t int32 0x6087 0x00 1000

#Servo ON.
ethercat download -p 0 -t uint16 0x6040 0x00 0x06
ethercat download -p 0 -t uint16 0x6040 0x00 0x07
ethercat download -p 0 -t uint16 0x6040 0x00 0x0f

#set 1000 torque.
ethercat download -p 0 -t int16 0x6071 0x00 1000

#set 0 torque to stop.
#ethercat download -p 0 -t int16 0x6071 0x00 0




