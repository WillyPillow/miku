#!/usr/bin/env bash

export PATH="app:bin:$PATH"

for (( pid=$1; pid<=$2; pid++ )); do
  echo "start get PID $pid"
  ./bin/tddump $pid
done
