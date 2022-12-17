#!/bin/bash
x=1
while [ $x -le 10 ]
do
  time -p ./main >> ./output/main.log
  x=$(( $x + 1 ))
done