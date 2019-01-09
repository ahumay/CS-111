#NAME: Anthony Humay
#EMAIL: ahumay@ucla.edu
#ID: 304731856
#SLIPDAYS: 1

#	1. test name
#	2. # threads
#	3. # iterations per thread
#	4. # lists
#	5. # operations performed (threads x iterations x (ins + lookup + delete))
#	6. run time (ns)
#	7. run time per operation (ns)
#	8. average wait time for locks for threads (ns)
#lab2b_1.png ... throughput vs. number of threads for mutex and spin-lock synchronized list operations.
#lab2b_2.png ... mean time per mutex wait and mean time per operation for mutex-synchronized list operations.
#lab2b_3.png ... successful iterations vs. threads for each synchronization method.
#lab2b_4.png ... throughput vs. number of threads for mutex synchronized partitioned lists.
#lab2b_5.png ... throughput vs. number of threads for spin-lock-synchronized partitioned lists.

set terminal png
set datafile separator ","

set title "List-1: Operations per second vs Threads"
set xlabel "Threads"
set logscale x 10
set ylabel "Operations per second"
set logscale y 10
set output 'lab2b_1.png'
plot \
	"< grep ''list-none-m,[0-8][0-4]*,1000,1,'' lab2b_list.csv" \
	using ($2):(1000000000)/($7) \
	title 'With Mutex' with linespoints lc rgb 'red', \
	"< grep ''list-none-s,[0-8][0-4]*,1000,1,'' lab2b_list.csv" \
	using ($2):(1000000000)/($7) \
	title 'With Spin' with linespoints lc rgb 'blue'

set title "List-2: Mean Time per Mutex Wait & Mean Time per Operation vs Threads"
set xlabel "Threads"
set logscale x 10
set ylabel "ns"
set logscale y 10
set output 'lab2b_2.png'
plot \
	"< grep ''list-none-m,[0-8][0-4]*,1000,1,'' lab2b_list.csv" \
	using ($2):($8) \
	title 'Average Time / Mutex Wait' with linespoints lc rgb 'red', \
	"< grep ''list-none-m,[0-8][0-4]*,1000,1,'' lab2b_list.csv" \
	using ($2):($7) \
	title 'Average Time / Operation' with linespoints lc rgb 'blue'

set title "List-3: Iteration Successes vs Threads"
set xlabel "Threads"
set logscale x 2
set ylabel "Iterations / Thread"
set logscale y 10
set output 'lab2b_3.png'
plot \
	"< grep 'list-id-none,[0-8][0-6]*,[1,2,4,8]6*,4' lab2b_list.csv" \
	using ($2):($3) \
	title "None" with points lc rgb 'red', \
	"< grep 'list-id-m,[0-8][0-6]*,[0-8]0,4' lab2b_list.csv" \
	using ($2):($3) \
	title "With Mutex" with points lc rgb 'blue' pt 5, \
	"< grep 'list-id-s,[0-8][0-6]*,[0-8]0,4' lab2b_list.csv" \
	using ($2):($3) \
	title "With Spin" with points lc rgb 'green'

set title "List-4: Aggregated Throughput (w/ Mutex) vs Threads"
set xlabel "Threads"
set logscale x 2
set ylabel "Total Operations per Second"
set logscale y 10
set output 'lab2b_4.png'
plot \
	"< grep 'list-none-m,[0-8]2*,1000,1,' lab2b_list.csv" \
	using ($2):(1000000000)/($7) \
	title 'W/ 1 list' with linespoints lc rgb 'green', \
	"< grep 'list-none-m,[0-8]2*,1000,4,' lab2b_list.csv" \
	using ($2):(1000000000)/($7) \
	title 'W/ 4 lists' with linespoints lc rgb 'red', \
	"< grep 'list-none-m,[0-8]2*,1000,8,' lab2b_list.csv" \
	using ($2):(1000000000)/($7) \
	title 'W/ 8 lists' with linespoints lc rgb 'blue', \
	"< grep 'list-none-m,[0-8]2*,1000,16,' lab2b_list.csv" \
	using ($2):(1000000000)/($7) \
	title 'W/ 16 lists' with linespoints lc rgb 'orange'

set title "List-5: Aggregated Throughput (w/ Spin) vs Threads"
set xlabel "Threads"
set logscale x 2
set ylabel "Total Operations per Second"
set logscale y 10
set output 'lab2b_5.png'
plot \
	"< grep 'list-none-s,[0-8]2*,1000,1,' lab2b_list.csv" \
	using ($2):(1000000000)/($7) \
	title 'W/ 1 list' with linespoints lc rgb 'green', \
	"< grep 'list-none-s,[0-8]2*,1000,4,' lab2b_list.csv" \
	using ($2):(1000000000)/($7) \
	title 'W/ 4 lists' with linespoints lc rgb 'red', \
	"< grep 'list-none-s,[0-8]2*,1000,8,' lab2b_list.csv" \
	using ($2):(1000000000)/($7) \
	title 'W/ 8 lists' with linespoints lc rgb 'blue', \
	"< grep 'list-none-s,[0-8]2*,1000,16,' lab2b_list.csv" \
	using ($2):(1000000000)/($7) \
	title 'W/ 16 lists' with linespoints lc rgb 'orange'