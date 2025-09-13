#!/bin/bash

# 디바이스 ID 범위 설정
START_ID=108
END_ID=117

BROKER_URL="emqx-nlb-bfefab8efb8db52f.elb.ap-northeast-2.amazonaws.com"
BROKER_PORT=1883

echo "Starting simulators with DEVICE_ID from $START_ID to $END_ID..."

for i in $(seq $START_ID $END_ID)
do
  export DEVICE_ID=$i
  export BROKER_URL=$BROKER_URL
  export BROKER_PORT=$BROKER_PORT
  
  LOG_FILE="simulator_${DEVICE_ID}.log"
  
  echo "Starting simulator with DEVICE_ID=$DEVICE_ID, logging to $LOG_FILE"
  
  python3 main.py > "$LOG_FILE" 2>&1 &
done

echo "All simulators started in the background."
echo "To stop them, run 'kill_simulators.sh'."
