#!/bin/bash
#2020/9/3 zhangshaoyan +8613522296239 TechServo RJS17,Profile Position Mode.
#clear fault.
ethercat download -p 0 -t uint16 0x6040 0x00 0x80

#Sync Manager2,Synchronization Type=0:Free Run.
ethercat download -p 0 -t uint16 0x1c32 0x01 0
ethercat states -p 0 OP

#Mode of Operation=1.
ethercat download -p 0 -t int8 0x6060 0x00 1

#Minimum Software Position Limit.
#ethercat download -p 0 -t int32 0x607D 0x01 0xFFFFFFFF
#Maximum Software Position Limit.
#ethercat download -p 0 -t int32 0x607D 0x02 0x0FFFFFFF

#Profile Velocity.
ethercat download -p 0 -t uint32 0x6081 0x00 600000
#Profile Acceleration.
ethercat download -p 0 -t uint32 0x6083 0x00 10000
#Profile Deceleration.
ethercat download -p 0 -t uint32 0x6084 0x00 10000
#Quick Stop Deceleration.
ethercat download -p 0 -t uint32 0x6085 0x00 10000
#Motion Profile Type=T.
ethercat download -p 0 -t int16 0x6086 0x00 0


#Servo ON.
ethercat download -p 0 -t uint16 0x6040 0x00 0x06
ethercat download -p 0 -t uint16 0x6040 0x00 0x0f

#get current position.
ethercat upload -p 0 -t int32 0x6064 0x00

#set target position.
ethercat download -p 0 -t int32 0x607A 0x00 0
#ethercat download -p 0 -t int32 0x607A 0x00 -- -200000
#ethercat download -p 0 -t int32 0x607A 0x00 600000


#Control Word (absolute position).
ethercat download -p 0 -t uint16 0x6040 0x00 0x1F

