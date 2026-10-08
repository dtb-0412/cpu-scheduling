#include"scheduling.h"

int main() {
	/*
	Input file format:
	1st line:	<number of processes> <round robin time quantum>
	n-th line:	<process name> <arrival time> <cpu burst time> <priority level>

	Priority level 1 is highest, then level 2, 3 and so on.
	*/
	ScheduleInfo info;
	if (!read_input_file("Input.txt", info)) {
		return -1;
	}

	// Non-preemptive First Come First Serve
	std::cout << "Non-preemptive First Come First Serve:\n";
	first_come_first_serve(info);

	if (write_output_file("FCFS.txt", info)) {
		std::cout << "Success\n\n";
	}

	// Preemptive Round Robin
	std::cout << "Preemptive Round Robin:\n";
	round_robin(info);

	if (write_output_file("RR.txt", info)) {
		std::cout << "Success\n\n";
	}

	// Non-preemptive Shortest Job First
	std::cout << "Non-preemptive Shortest Job First:\n";
	shortest_job_first(info);

	if (write_output_file("SJF.txt", info)) {
		std::cout << "Success\n\n";
	}

	// Preemptive Priority Scheduling
	std::cout << "Preemptive Priority Scheduling:\n";
	priority_scheduling(info);

	if (write_output_file("Priority.txt", info)) {
		std::cout << "Success\n\n";
	}
	return 0;
}