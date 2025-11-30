#pragma once
#include<algorithm>
#include<fstream>
#include<iomanip>
#include<iostream>
#include<numeric>
#include<queue>
#include<set>
#include<vector>

using namespace std;

struct Process {
	bool operator<(const Process& other) const {
		return arrival < other.arrival;
	}

	string name{};
	uint64_t arrival	= 0; // Arrival time
	uint64_t burst		= 0; // CPU burst time
	uint64_t priority	= 0; // Priority level
	uint64_t turnaround	= 0; // Turnaround time
	uint64_t waiting	= 0; // Waiting time
};

struct ScheduleInfo {
	ScheduleInfo() = default;

	ScheduleInfo(const ScheduleInfo&) = delete;

	vector<pair<string, uint64_t>> chart{}; // Gantt chart: Process name - Finish time
	vector<Process> process{};	// Processes sorted in order of their arrival time
	uint64_t quantum = 0;		// Time quantum for Round Robin
};

bool readInputFile(const string& filepath, ScheduleInfo& info) {
	ifstream ifs;
	ifs.open(filepath, ios::in);
	if (!ifs.is_open()) {
		cout << "Cannot open file \"" << filepath << "\"\n";
		return false;
	}

	size_t size = 0;
	ifs >> size >> info.quantum;
	
	info.process.reserve(size);
	for (size_t i = 0; i < size; ++i) {
		Process newProcess;
		ifs >> newProcess.name >> newProcess.arrival >> newProcess.burst >> newProcess.priority;
		info.process.push_back(newProcess);
	}
	sort(info.process.begin(), info.process.end());

	ifs.close();
	return true;
}

bool writeOutputFile(const string& filepath, const ScheduleInfo& info) {
	ofstream ofs;
	ofs.open(filepath, ios::out | ios::trunc); // Open file and overwrite entirely, or create new file if not exists
	if (!ofs.is_open()) {
		cout << "Cannot open file \"" << filepath << "\"\n";
		return false;
	}
	// Gantt chart
	ofs << "Scheduling chart: " << 0;
	for (const auto& [process, time] : info.chart) {
		ofs << " ~" << process << "~ " << time;
	}
	ofs << "\n\n";
	// Each process's turnaround and waiting time
	for (const auto& process : info.process) {
		ofs << setw(10) << left << (process.name + ": ") 
			<< "TT = " << setw(10) << left << process.turnaround 
			<< "WT = " << process.waiting << "\n";
	}
	// Average turnaround and waiting time
	const double totalTurnaround = accumulate(info.process.begin(), info.process.end(), 0.0,
		[](const auto& a, const Process& b) { return a + b.turnaround; }
	);
	const double totalWaiting = accumulate(info.process.begin(), info.process.end(), 0.0,
		[](const auto& a, const Process& b) { return a + b.waiting; }
	);
	ofs << setw(10) << left << "Average: " 
		<< "TT = " << setw(10) << left << setprecision(2) << fixed << totalTurnaround / info.process.size()
		<< "WT = " << totalWaiting / info.process.size();
	
	ofs.close();
	return true;
}

void fcfs(ScheduleInfo& info) {
	// Perform FCFS (First Come, First Serve) algorithm
	info.chart.clear();

	uint64_t time = 0;
	for (auto i = info.process.begin(); i != info.process.end(); ++i) {
		time += i->burst;
		info.chart.push_back({ i->name, time });

		i->turnaround	= time - i->arrival;
		i->waiting		= i->turnaround - i->burst;
	}
}

void rr(ScheduleInfo& info) {
	// Perform RR (Round Robin) algorithm
	info.chart.clear();
	// Store original burst time. Values are restored after the algorithm has finished
	vector<uint64_t> burst(info.process.size());
	transform(info.process.begin(), info.process.end(), burst.begin(),
		[](const auto pc) { return pc.burst; }
	);
	// Ready queue to cycle processes after each time quantum
	queue<uint64_t> ready;

	uint64_t time	= 0;
	uint64_t index	= 0;
	ready.push(index);
	while (!ready.empty()) {
		const auto currIdx = ready.front();
		ready.pop();

		Process& currPc = info.process[currIdx];
		if (ready.empty() && index == info.process.size() - 1) { // Final process, run through all the remaining time
			time += currPc.burst;
			info.chart.push_back({ currPc.name, time });

			currPc.turnaround	= time - currPc.arrival;
			currPc.waiting		= currPc.turnaround - burst[currIdx];
			break;
		}

		const auto runTime = min(info.quantum, currPc.burst);
		time += runTime;
		info.chart.push_back({ currPc.name, time });
		// Add arrived processes to ready queue
		while (index + 1 < info.process.size() && info.process[index + 1].arrival <= time) {
			ready.push(++index);
		}
		// Add current process back to ready queue if it has not finished, otherwise calculate turnaround and waiting time
		if ((currPc.burst -= runTime)) {
			ready.push(currIdx);
		} else {
			currPc.turnaround	= time - currPc.arrival;
			currPc.waiting		= currPc.turnaround - burst[currIdx];
		}
	}
	// Restore original values
	for (auto i = 0; i < info.process.size(); ++i) {
		info.process[i].burst = burst[i];
	}
}

void sjf(ScheduleInfo& info) {
	// Perform SJF (Shortest Job First) algorithm
	info.chart.clear();
	// Ready queue using multiset to maintain order of burst time, while allowing duplicate values
	const auto compare = [&info](const auto lhs, const auto rhs) -> bool {
		return info.process[lhs].burst < info.process[rhs].burst;
	};
	multiset<uint64_t, decltype(compare)> ready(compare);
	
	uint64_t time	= 0;
	uint64_t index	= 0;
	ready.insert(index);
	while (!ready.empty()) {
		Process& currPc = info.process[ready.extract(ready.begin()).value()];

		time += currPc.burst;
		info.chart.push_back({ currPc.name, time });

		currPc.turnaround	= time - currPc.arrival;
		currPc.waiting		= currPc.turnaround - currPc.burst;
		// Add arrived processes to ready queue in ascending order of CPU burst
		while (index + 1 < info.process.size() && info.process[index + 1].arrival <= time) {
			ready.insert(++index);
		}
	}
}

void ps(ScheduleInfo& info) {
	// Perform preemptive PS (Priority Scheduling) algorithm
	info.chart.clear();
	// Store original arrival and burst time. Values are restored after the algorithm has finished
	vector<uint64_t> burst(info.process.size()), arrival(info.process.size());
	transform(info.process.begin(), info.process.end(), burst.begin(),
		[](const auto pc) { return pc.burst; }
	);
	transform(info.process.begin(), info.process.end(), arrival.begin(),
		[](const auto pc) { return pc.arrival; }
	);
	// Ready queue using multiset to maintain order of arrival time, while allowing duplicate values
	const auto compare = [&info](const auto lhs, const auto rhs) -> bool {
		return info.process[lhs].arrival < info.process[rhs].arrival;
	};
	multiset<uint64_t, decltype(compare)> ready(compare);
	for (uint64_t i = 0; i < info.process.size(); ++i) {
		ready.insert(i);
	}

	uint64_t time = 0;
	while (!ready.empty()) {
		// Find the process with highest priority among those arrived to run
		auto min = ready.begin();
		for (auto i = next(ready.begin()); i != ready.end(); ++i) {
			const Process& pc = info.process[*i];
			if (time < pc.arrival) {
				break;
			}
			if (pc.priority < info.process[*min].priority) {
				min = i;
			}
		}

		const auto nh = ready.extract(min); // Extract and store so that we can insert it back if needed
		const auto currIdx = nh.value();
		Process& currPc = info.process[currIdx];

		// Calculate run time by checking if there is any interruption by a higher priority process
		uint64_t runTime = currPc.burst;
		for (auto i = ready.begin(); i != ready.end(); ++i) {
			const Process& nextPc = info.process[*i];
			if (nextPc.arrival >= time + runTime) { // No interruption, run through its entire burst time
				break;
			}

			if (nextPc.priority < currPc.priority) { // Interrupted, limit its run time to until the higher priority process arrives
				runTime = nextPc.arrival - time;
				// Shift its arrival time backward, hypothetically, it can be resumed when the higher priority process ends.
				// If not, when another higher priority process has arrived, that process will be chosen to run first,
				// delaying the current lower priority process accordingly.
				currPc.arrival = nextPc.arrival + burst[*i];
				break;
			}
		}

		time += runTime;
		info.chart.push_back({ currPc.name, time });
		// Add current process back to ready queue if it has not finished, otherwise calculate turnaround and waiting time
		if ((currPc.burst -= runTime)) {
			ready.insert(nh.value());
		} else {
			currPc.turnaround	= time - arrival[currIdx];
			currPc.waiting		= currPc.turnaround - burst[currIdx];
		}
	}
	// Restore original values
	for (auto i = 0; i < info.process.size(); ++i) {
		info.process[i].arrival = arrival[i];
		info.process[i].burst	= burst[i];
	}
}