#!/bin/bash
#TechServo RJS17 Profile Velocity Mode.
#2020/9/3,zhangshaoyan +8613522296239.

#clear fault.
#bit7=1.
#Reset Fault.A low-to-high transition of this bit makes the amplifier attempt to clear any latched fault condition.
ethercat download -p 0 -t uint16 0x6040 0x00 0x80

#Sync Manager2,Synchronization Type=0:Free Run.
ethercat download -p 0 -t uint16 0x1c32 0x01 0
ethercat states -p 0 OP

#Mode of Operation=3,Profile Velocity mode.
ethercat download -p 0 -t uint8 0x6060 0x00 3

#Profile Acceleration.
ethercat download -p 0 -t int32 0x6083 0x00 1000
#Profile Deceleration.
ethercat download -p 0 -t int32 0x6084 0x00 1000

#Servo ON.
ethercat download -p 0 -t uint16 0x6040 0x00 0x06
ethercat download -p 0 -t uint16 0x6040 0x00 0x07
ethercat download -p 0 -t uint16 0x6040 0x00 0x0f

#Target Velocity.
ethercat download -p 0 -t int32 0x60FF 0x00 500000

#we can change velocity dynamic with following commands.
#if the velocity was set too big will cause Amplifier error,motor stops.
#ethercat download -p 0 -t int32 0x60FF 0x00 100000
#ethercat download -p 0 -t int32 0x60FF 0x00 5000



