#include"scheduling.h"

int main() {
	ScheduleInfo info;
	readInputFile("Input.txt", info);

	fcfs(info); // Non-preemptive First Come First Serve
	writeOutputFile("FCFS.txt", info);

	rr(info); // Preemptive Round Robin
	writeOutputFile("RR.txt", info);

	sjf(info); // Non-preemptive Shortest Job First
	writeOutputFile("SJF.txt", info);

	ps(info); // Preemptive Priority Scheduling
	writeOutputFile("Priority.txt", info);
	return 0;
}