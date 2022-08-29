#!/bin/bash
x=1
while [ $x -le 5 ]
do
  ./main >> ./data/main.log
  x=$(( $x + 1 ))
done
