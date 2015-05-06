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

# for i in $(pgrep server); do ps -mo pid,tid,fname,user,psr,sgi_p,%cpu,c,class,sched,wchan -p $i;done

# meanning of the cols
# pid:	process ID number of the process.
# tid:	alias lwp: lwp (light weight process, or thread) ID of the lwp being reported. (alias spid, tid).
# fname:(COMMAND) first 8 bytes of the base name of the process's executable file. The output in this column may contain spaces.
# user:	effective user name. This will be the textual user ID, if it can be obtained and the field width permits, or a decimal representation otherwise.
# psr:	processor that process is currently assigned to.
# sgi_p:processor that the process is currently executing on. Displays "*" if the process is not currently running or runnable.
# %cpu:	cpu utilization of the process in "##.#" format. Currently, it is the CPU time used divided by the time the process has been running (cputime/realtime ratio), expressed as a percentage. It will not add up to 100% unless you are lucky. (alias pcpu).
# class:scheduling class of the process. (alias policy, cls). Field's possible values are:
		#    -	not reported
		#    TS	SCHED_OTHER
		#    FF	SCHED_FIFO
		#    RR	SCHED_RR
		#    ?	unknown value
# sched:scheduling policy of the process. The policies sched_other, sched_fifo, and sched_rr are respectively displayed as 0, 1, and 2
# wchan:name of the kernel function in which the process is sleeping, a "-" if the process is running, or a "*" if the process is multi-threaded and ps is not displaying threads.

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