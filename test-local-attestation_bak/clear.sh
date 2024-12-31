#!/bin/bash

ps -aux | grep KII
echo Killing all KII..

killall -r KII
ps -aux | grep KII

echo ..killed

echo Cleaning make..
make clean

echo Re-initializing player directories..

rm -r Player-Data
mkdir Player-Data
cd Player-Data
mkdir 2-2-40
mkdir 2-p-128
cd ..
echo DONE.

echo Clearing player logs..
rm player_0.log
rm player_1.log
echo ..Done
echo Clearing Verifier logs..
rm kii_0.log
rm kii_1.log
echo ..Done
