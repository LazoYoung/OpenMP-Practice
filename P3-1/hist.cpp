#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
#include <functional>
#include <cmath>
#include "omp.h"
#include "util.h"
#include "hist.h"

#include <random>

using namespace std;

constexpr int data_count = 1024 * 1024 * 1024;
constexpr int versions = 4;
constexpr float r_min = 0;
constexpr float r_max = 10;
constexpr float r_range = r_max - r_min;
const Algorithm algo[versions] = {
		serialized, synchronized, localBin, reduction
};
const string hist_name[versions] = {
	"v0_serial", "v1_sync", "v2_localBin", "v3_reduce"
};

int main() {
	float bin_size;
	readNumber(bin_size, "Size of each bin", 0.00001f, r_max);
	const int bin_count = static_cast<int>(ceil(r_range / bin_size));
	vector<float> data = generateNumbers(data_count);

	cout << "Data: ";
	print(data, 10);
	cout << '\n';

	for (int i = 0; i < versions; ++i) {
		Histogram hist(hist_name[i], data, bin_size, bin_count);
		const auto seconds = benchmark(hist, algo[i]);
		cout << "Bins: ";
		print(hist.bin);
		cout << "Elapsed time: " << seconds << "s" << endl << endl;
	}

	return 0;
}

inline int getWorkerCount() {
	return omp_get_max_threads() - 1;
}

inline float getRandomFloat() {
	static random_device rd;
	static mt19937 engine(rd());
	static uniform_real_distribution dist(r_min, r_max);
	const float random = dist(engine);
	return random;
}

vector<float> generateNumbers(int count) {
	vector<float> data(count);

	auto worker = [&](Work& w) {
		const int workload = count / w.num_threads;
		const int start = w.thread_id * workload;
		const int end = (w.thread_id < w.num_threads - 1) ? (w.thread_id + 1) * workload : count;

		for (int i = start; i < end; ++i) {
			data[i] = getRandomFloat();
			++w.progress;
		}
	};

	cout << "Generating numbers... ";
	computeParallelWork(count, worker);
	return data;
}

void computeParallelWork(int target, const function<void(Work&)>& worker) {
	const int worker_count = getWorkerCount();
	vector progress_bin(worker_count, 0);

	auto get_progress = [&] {
		int progress = 0;
		for (int i = 0; i < worker_count; ++i) {
			progress += progress_bin[i];
		}
		return progress;
	};

	#pragma omp parallel num_threads(worker_count + 1)
	{
		if (omp_get_thread_num() == worker_count) {
			int last_width = 0;

			while (true) {
				if (last_width > 0) {
					cout << string(last_width, '\b') << string(last_width, ' ') << string(last_width, '\b');
				}

				const int progress = get_progress();
				const int percent = static_cast<int>(round(static_cast<double>(progress) / target * 100));
				const int width = numberOfDigits(progress) + numberOfDigits(percent) + 4;
				last_width = width;

				cout << progress << " (" << percent << "%)";

				if (progress < target) {
					this_thread::sleep_for(chrono::milliseconds(500));
				}
				else {
					cout << '\n';
					break;
				}
			}
		}
		else {
			int id = omp_get_thread_num();
			Work work(id, worker_count, progress_bin[id], get_progress);

			worker(work);
		}
	}
}

void serialized(Histogram& hist) {
	const int data_size = static_cast<int>(hist.data.size());

	auto worker = [&](Work& w) {
		if (w.thread_id != 0) {
			// effectively makes it serially processed
			return;
		}

		for (int i = 0; i < data_size; ++i) {
			const float number = hist.data[i];
			int idx = static_cast<int>(floor(number / hist.bin_size));
			idx = (idx < hist.bin_count) ? idx : hist.bin_count - 1;

			++hist.bin[idx];
			++w.progress;
		}
	};

	computeParallelWork(data_size, worker);
}

void synchronized(Histogram& hist) {
	const int data_size = static_cast<int>(hist.data.size());
	const int worker_count = getWorkerCount();

	auto worker = [&](Work& w) {
		const int workload = data_size / worker_count;
		const int start = w.thread_id * workload;
		const int end = (w.thread_id < worker_count - 1) ? (w.thread_id + 1) * workload : data_size;

		for (int i = start; i < end; ++i) {
			const float number = hist.data[i];
			int idx = static_cast<int>(floor(number / hist.bin_size));
			idx = (idx < hist.bin_count) ? idx : hist.bin_count - 1;

			#pragma omp atomic
			++hist.bin[idx];

			++w.progress;
		}
	};
	
	computeParallelWork(data_size, worker);
}

void localBin(Histogram& hist) {
	const int data_size = static_cast<int>(hist.data.size());
	const int worker_count = getWorkerCount();
	vector local_bin(worker_count * hist.bin_count, 0);

	auto worker = [&](Work& w) {
		const int workload = data_size / worker_count;
		const int start = w.thread_id * workload;
		const int end = (w.thread_id < worker_count - 1) ? (w.thread_id + 1) * workload : data_size;

		for (int i = start; i < end; ++i) {
			const float number = hist.data[i];
			int idx = static_cast<int>(floor(number / hist.bin_size));
			idx = (idx < hist.bin_count) ? idx : hist.bin_count - 1;

			++local_bin[w.thread_id * hist.bin_count + idx];
			++w.progress;
		}

		if (w.thread_id == 0) { // only 1 thread runs here
			while (w.get_total_progress() < data_size) {
				this_thread::sleep_for(chrono::milliseconds(100));
			}

			// combine local bins
			for (int b = 0; b < hist.bin_count; ++b) {
				int count = 0;

				for (int t = 0; t < worker_count; ++t) {
					count += local_bin[t * hist.bin_count + b];
				}

				hist.bin[b] = count;
			}

			++w.progress;
		}
	};

	computeParallelWork(data_size + 1, worker);
}

void reduction(Histogram& hist) {
	
}

long long benchmark(Histogram& hist, const Algorithm& algorithm) {
	cout << '[' << hist.name << "] Filling up... ";
	const auto start_point = chrono::system_clock::now();
	algorithm(hist);
	const auto end_point = chrono::system_clock::now();
	return chrono::duration_cast<chrono::seconds>(end_point - start_point).count();
}
