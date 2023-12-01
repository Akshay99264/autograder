#!/bin/bash

num_clients=$1
loop_num=$2
sleep_time=$3
timeout=$4

if [[ $# != 4 ]]; then
    echo Usage: ./loadtest.sh \<num_clients\> \<loop_num\> \<sleep_time\> \<timeout\>
fi

> cpu_utilization.csv
> threads.csv

command="./client 127.0.0.1 8080 test.c $loop_num $sleep_time $timeout"

echo "Clients No., Average response time,Throughput,Goodput,Timeout rate,Error Rate,Request Rate,Successfull response,Loop time" > analysis_data.csv

for j in 1 2 4 8 16 32 64 128 140 150 160 170 180 200
do
    vmstat 1 | awk -v i="$j" 'BEGIN {OFS=" "; FS=" "}  {print i,$0}' | sed -E 's/[ ]+/,/g' >> cpu_utilization.csv &
    for (( i=1; i<=$j; i++ ));
    do
        $command | tail -1 | awk -v i="$i" 'BEGIN {OFS=","} {print i,$0}' >> analysis_data.csv &
    done

    for ((i=0; i<10; i++)); do
        thread_counts=$(ps -eLf| grep "./server 8080" | wc -l)
        echo $j,$thread_counts >> threads.csv
        sleep 1
    done
    killall vmstat

    wait
    echo ,,,,,,,,, >> analysis_data.csv
    echo ,, >> threads.csv
    # num_clients=$num_clients+10
    rm files/*
done

# python3 cpu_utilization_graph.py
# python3 graphs.py