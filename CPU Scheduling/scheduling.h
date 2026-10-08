#pragma once
#include<algorithm>
#include<filesystem>
#include<fstream>
#include<iomanip>
#include<iostream>
#include<numeric>
#include<queue>
#include<set>
#include<vector>

inline constexpr std::size_t MAX_PROCESSES = 10000;

struct Process {
	bool operator<(const Process& other) const { // Compare arrival time
		return arrival < other.arrival;
	}

	std::string name{};
	std::size_t arrival		= 0; // Arrival time
	std::size_t burst		= 0; // CPU burst time
	std::size_t priority	= 0; // Priority level
	std::size_t turnaround	= 0; // Turnaround time
	std::size_t waiting		= 0; // Waiting time
};

struct ScheduleInfo {
	ScheduleInfo() = default;

	ScheduleInfo(const ScheduleInfo&) = delete;

	// Gantt chart: Process name - Finish time
	std::vector<std::pair<std::string, std::size_t>> chart{};

	/*
	Processes sorted in order of their arrival time.
	We use vector instead of multiset because of the need for modification and random access, which multiset
	does not support.
	*/
	std::vector<Process> processes{};
	// Time quantum for Round Robin
	std::size_t quantum = 0;
};

inline bool read_input_file(std::string_view filepath, ScheduleInfo& info) {
	std::ifstream ifs;
	ifs.open(filepath, std::ios::in);
	if (!ifs.is_open()) {
		std::cout << "Cannot open file \"" << filepath << "\"\n";
		return false;
	}

	uint64_t count = 0;

	int64_t quantum = 0;
	if (!(ifs >> count >> quantum) ||
		count == 0 || count > MAX_PROCESSES || quantum <= 0)
	{
		std::cout << "Invalid header in file \"" << filepath << "\"\n";
		return false;
	}
	info.quantum = static_cast<std::size_t>(quantum);
	
	info.processes.clear();
	info.processes.reserve(count);
	for (std::size_t i = 0; i < count; ++i) {
		Process newProcess;

		int64_t arrival = 0, burst = 0, priority = 0;
		if (!(ifs >> newProcess.name >> arrival >> burst >> priority) ||
			arrival < 0 || burst <= 0 || priority < 0)
		{
			std::cout << "Invalid or missing data for process #" << i + 1 << "\n";
			info.processes.clear();
			return false;
		}

		newProcess.arrival	= static_cast<std::size_t>(arrival);
		newProcess.burst	= static_cast<std::size_t>(burst);
		newProcess.priority = static_cast<std::size_t>(priority);
		info.processes.push_back(std::move(newProcess));
	}
	std::stable_sort(info.processes.begin(), info.processes.end());

	ifs.close();
	return true;
}

inline bool write_output_file(std::string_view filepath, const ScheduleInfo& info) {
	std::ofstream ofs;
	ofs.open(filepath, std::ios::out | std::ios::trunc); // Open file and overwrite entirely, or create new file if not exists
	if (!ofs.is_open()) {
		std::cout << "Cannot open file \"" << filepath << "\"\n";
		return false;
	}
	// Gantt chart
	ofs << "Scheduling: " << 0;
	for (const auto& [process, finishTime] : info.chart) {
		ofs << " [" << process << "] " << finishTime;
	}
	ofs << "\n\n";
	// Each process's turnaround and waiting time
	for (const auto& process : info.processes) {
		ofs << std::setw(10) << std::left << (process.name + ": ") 
			<< "TT = " << std::setw(10) << std::left << process.turnaround 
			<< "WT = " << process.waiting << "\n";
	}
	// Average turnaround and waiting time
	if (!info.processes.empty()) {
		const double totalTurnaround = std::accumulate(info.processes.begin(), info.processes.end(), 0.0,
			[](const auto& a, const Process& b) { return a + b.turnaround; }
		);
		const double totalWaiting = std::accumulate(info.processes.begin(), info.processes.end(), 0.0,
			[](const auto& a, const Process& b) { return a + b.waiting; }
		);
		ofs << std::setw(10) << std::left << "Average: "
			<< "TT = " << std::setw(10) << std::left << std::setprecision(2) << std::fixed << totalTurnaround / info.processes.size()
			<< "WT = " << totalWaiting / info.processes.size();
	}
	
	ofs.close();
	return !ofs.fail();
}

inline void first_come_first_serve(ScheduleInfo& info) {
	// Perform FCFS (First Come, First Serve) algorithm
	info.chart.clear();

	std::size_t time = 0;
	for (auto& process : info.processes) {
		time = std::max(time, process.arrival) + process.burst;
		info.chart.push_back({ process.name, time });

		process.turnaround	= time - process.arrival;
		process.waiting		= process.turnaround - process.burst;
	}
}

inline void round_robin(ScheduleInfo& info) {
	// Perform RR (Round Robin) algorithm
	if (info.processes.empty()) {
		return;
	}
	info.chart.clear();

	const auto quantum = info.quantum > 0
		? info.quantum
		: std::numeric_limits<std::size_t>::max();

	std::vector<std::size_t> processBursts(info.processes.size());
	std::transform(info.processes.begin(), info.processes.end(), processBursts.begin(),
	   [](const auto& process) { return process.burst; }
	);
	// Ready queue to cycle processes after each time quantum
	std::queue<std::size_t> ready;

	std::size_t index	= 0;
	std::size_t time	= info.processes.front().arrival;

	const auto count = info.processes.size();

	ready.push(index);
	while (!ready.empty()) {
		const auto currIndex = ready.front();
		ready.pop();

		auto& currProcess	= info.processes[currIndex];
		auto& currBurst		= processBursts[currIndex];

		if (ready.empty() && index == count - 1) { // Final process, run through all the remaining time
			time += currBurst;
			info.chart.push_back({ currProcess.name, time });

			currProcess.turnaround	= time - currProcess.arrival;
			currProcess.waiting		= currProcess.turnaround - currProcess.burst;
			break;
		}

		const auto runTime = std::min(currBurst, quantum);
		
		time += runTime;
		info.chart.push_back({ currProcess.name, time });
		// Add arrived processes to ready queue
		while (index + 1 < count && info.processes[index + 1].arrival <= time) {
			ready.push(++index);
		}
		// Add current process back to ready queue if it has not finished, otherwise calculate turnaround and waiting time
		currBurst -= runTime;
		if (currBurst > 0) {
			ready.push(currIndex);
		}
		else {
			currProcess.turnaround	= time - currProcess.arrival;
			currProcess.waiting		= currProcess.turnaround - currProcess.burst;
		}
		// Handle CPU idling
		if (ready.empty() && index + 1 < count) {
			++index;

			time = info.processes[index].arrival;
			ready.push(index);
		}
	}
}

inline void shortest_job_first(ScheduleInfo& info) {
	// Perform SJF (Shortest Job First) algorithm
	if (info.processes.empty()) {
		return;
	}
	info.chart.clear();
	// Priority queue (min-heap) is the most suitable container for this algorithm
	auto compare = [&info](const std::size_t lhs, const std::size_t rhs) -> bool {
		if (info.processes[lhs].burst != info.processes[rhs].burst) {
			return info.processes[lhs].burst > info.processes[rhs].burst;
		}
		return lhs > rhs; // Compare index if same burst
	};
	std::priority_queue<std::size_t, std::vector<std::size_t>, decltype(compare)> ready(compare);

	std::size_t index	= 0;
	std::size_t time	= info.processes.front().arrival;

	const auto count = info.processes.size();

	ready.push(index);
	while (!ready.empty()) {
		auto& currProcess = info.processes[ready.top()];
		ready.pop();

		time += currProcess.burst;
		info.chart.push_back({ currProcess.name, time });

		currProcess.turnaround	= time - currProcess.arrival;
		currProcess.waiting		= currProcess.turnaround - currProcess.burst;
		// Add arrived processes to ready queue in ascending order of CPU burst
		while (index + 1 < count && info.processes[index + 1].arrival <= time) {
			ready.push(++index);
		}

		if (ready.empty() && index + 1 < count) {
			++index;

			time = info.processes[index].arrival;
			ready.push(index);
		}
	}
}

inline void priority_scheduling(ScheduleInfo& info) {
	// Perform preemptive PS (Priority Scheduling) algorithm
	if (info.processes.empty()) {
		return;
	}
	info.chart.clear();

	std::vector<std::size_t> processBursts(info.processes.size());
	std::transform(info.processes.begin(), info.processes.end(), processBursts.begin(),
	   [](const auto& process) { return process.burst; }
	);
	
	auto compare = [&info](const std::size_t lhs, const std::size_t rhs) -> bool {
		if (info.processes[lhs].priority != info.processes[rhs].priority) {
			return info.processes[lhs].priority > info.processes[rhs].priority;
		}
	};
	std::priority_queue<std::size_t, std::vector<std::size_t>, decltype(compare)> ready(compare);

	std::size_t index	= 0;
	std::size_t time	= info.processes.front().arrival;

	const auto count = info.processes.size();

	ready.push(index);
	while (!ready.empty()) {
		std::size_t currIndex = ready.top();
		ready.pop();

		auto& currProcess	= info.processes[currIndex];
		auto& currBurst		= processBursts[currIndex];

		const auto nextArrival = (index + 1 < count)
			? info.processes[index + 1].arrival
			: std::numeric_limits<std::size_t>::max();
		const auto timeDelta = (nextArrival > time)
			? nextArrival - time
			: std::numeric_limits<std::size_t>::max();

		auto runTime = std::min(currBurst, timeDelta);
		if (nextArrival >= time + currBurst) {
			runTime = currBurst;
		}

		time += runTime;
		currBurst -= runTime;
		if (!info.chart.empty() && info.chart.back().first == currProcess.name) {
			info.chart.back().second = time;
		}
		else {
			info.chart.push_back({ currProcess.name, time });
		}

		while (index + 1 < count && info.processes[index + 1].arrival <= time) {
			ready.push(++index);
		}

		if (currBurst > 0) {
			ready.push(currIndex);
		}
		else {
			currProcess.turnaround	= time - currProcess.arrival;
			currProcess.waiting		= currProcess.turnaround - currProcess.burst;
		}

		if (ready.empty() && index + 1 < info.processes.size()) {
			++index;
			
			time = info.processes[index].arrival;
			ready.push(index);
		}
	}
}