#!/bin/bash

MAX_PACKS=1000000

echo "Compilando..."
make all
echo "Done"

# Limpiar Ftrace
echo nop > /sys/kernel/debug/tracing/current_tracer
echo 0 > /sys/kernel/debug/tracing/tracing_on
#echo function_graph > /sys/kernel/debug/tracing/current_tracer
echo function > /sys/kernel/debug/tracing/current_tracer

num_port=1820
num_threads=1
num_clients=4
./server $MAX_PACKS $num_threads $num_port &
pid=$!
sleep 1
for ((i=1 ; $i<=$num_clients ; i++))
{
	./client $(($MAX_PACKS*10)) 127.0.0.1 $num_port > /dev/null &
}
wait $pid

make clean

cat /sys/kernel/debug/tracing/trace > "Trace_Sockets_"$num_threads"Threads_UDP.txt"

> /sys/kernel/debug/tracing/set_ftrace_pid

echo "Done"