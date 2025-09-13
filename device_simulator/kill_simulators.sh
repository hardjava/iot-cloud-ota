#!/bin/bash

echo "Stopping all device_simulator (main.py) processes..."

# 'python3 main.py'로 실행된 모든 프로세스를 찾아 종료
pkill -f "python3 main.py"

echo "Cleaning up log files..."
rm -f simulator_*.log

echo "All simulator processes terminated."
