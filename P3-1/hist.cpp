#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
#include <functional>
#include <random>
#include <cmath>
#include "omp.h"
#include "util.h"
#include "hist.h"

using namespace std;

constexpr int data_count = 1024 * 1024 * 1024;
constexpr float r_min = 0;
constexpr float r_max = 10;
constexpr float r_range = r_max - r_min;
const vector<Algorithm> version = {
	serialized,
	synchronized,
	localBin,
	reduction
};
const string hist_name[] = {
	"v0_serial",
	"v1_sync",
	"v2_localBin",
	"v3_reduce"
};

int main() {
	float bin_size;
	readNumber(bin_size, "Size of each bin", 0.00001f, r_max);
	const int bin_count = static_cast<int>(ceil(r_range / bin_size));
	vector<float> data = generateNumbers(data_count);

	cout << "Data: ";
	print(data, 10);
	cout << '\n';

	for (size_t i = 0; i < version.size(); ++i) {
		Histogram hist(hist_name[i], data, bin_size, bin_count);
		const auto seconds = benchmark(hist, version[i]);
		cout << "Bins: ";
		print(hist.bin);
		cout << "Elapsed time: " << seconds << "s\n\n";
	}

	return 0;
}

inline int getReduceCount(int init_size) {
	double size = init_size;
	int count = 0;
	
	while (size > 1.0) {
		size = ceil(size / 2);
		++count;
	}
	return count;
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

	auto worker = [&](const Work& w) {
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

void computeParallelWork(int target, const function<void(Work&)>& worker, bool track) {
	const int worker_count = getWorkerCount();
	const int thread_count = track ? worker_count + 1 : worker_count;
	vector progress_bin(worker_count, 0);

	auto get_progress = [&] {
		int progress = 0;
		for (int i = 0; i < worker_count; ++i) {
			progress += progress_bin[i];
		}
		return progress;
	};

	#pragma omp parallel num_threads(thread_count)
	{
		const int thread_id = omp_get_thread_num();
		
		if (track && thread_id == worker_count) {
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

		if (!track || thread_id < worker_count) {
			Work work(thread_id, worker_count, progress_bin[thread_id], get_progress);
			worker(work);
		}
	}
}

void serialized(Histogram& hist) {
	const int data_size = static_cast<int>(hist.data.size());

	auto worker = [&](const Work& w) {
		if (w.thread_id != 0) {
			// effectively makes it serially processed
			return;
		}

		for (int i = 0; i < data_size; ++i) {
			const float number = hist.data[i];
			int idx = static_cast<int>(floor(number / hist.bin_size));
			idx = idx < hist.bin_count ? idx : hist.bin_count - 1;

			++hist.bin[idx];
			++w.progress;
		}
	};

	computeParallelWork(data_size, worker);
}

void synchronized(Histogram& hist) {
	const int data_size = static_cast<int>(hist.data.size());
	const int worker_count = getWorkerCount();

	auto worker = [&](const Work& w) {
		const int workload = data_size / worker_count;
		const int start = w.thread_id * workload;
		const int end = (w.thread_id < worker_count - 1) ? (w.thread_id + 1) * workload : data_size;

		for (int i = start; i < end; ++i) {
			const float number = hist.data[i];
			int idx = static_cast<int>(floor(number / hist.bin_size));
			idx = idx < hist.bin_count ? idx : hist.bin_count - 1;

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

	auto worker = [&](const Work& w) {
		vector local_bin(hist.bin_count, 0);
		const int workload = data_size / worker_count;
		const int start = w.thread_id * workload;
		const int end = w.thread_id < worker_count - 1 ? (w.thread_id + 1) * workload : data_size;

		// populate local bin with numbers
		for (int i = start; i < end; ++i) {
			const float number = hist.data[i];
			int idx = static_cast<int>(floor(number / hist.bin_size));
			idx = idx < hist.bin_count ? idx : hist.bin_count - 1;

			++local_bin[idx];
			++w.progress;
		}

		// aggregate
		for (int i = 0; i < hist.bin_count; ++i) {
			#pragma omp critical
			{
				hist.bin[i] += local_bin[i];
			}
		}
	};

	computeParallelWork(data_size, worker);
}

void reduction(Histogram& hist) {
	int epoch = 0;
	int worker_count = getWorkerCount();
	const int data_size = static_cast<int>(hist.data.size());
	const int reduce_count = getReduceCount(worker_count);
	vector local_bin(hist.bin_count * worker_count, 0);
	
	auto worker1 = [&] (const Work& w) {
		const int workload = data_size / worker_count;
		const int start = w.thread_id * workload;
		const int end = w.thread_id < worker_count - 1 ? (w.thread_id + 1) * workload : data_size;
		const int offset = w.thread_id * hist.bin_count;

		// populate local bin with numbers
		for (int i = start; i < end; ++i) {
			const float number = hist.data[i];
			int idx = static_cast<int>(floor(number / hist.bin_size));
			idx = idx < hist.bin_count ? idx : hist.bin_count - 1;

			++local_bin[offset + idx];
			++w.progress;
		}
	};
	
	auto worker2 = [&] (const Work& w) {
		// perform reduction
		while (true) {
			#pragma omp barrier
			#pragma omp single
			{
				worker_count = worker_count > 1 ? static_cast<int>(ceil(worker_count / 2.0)) : 0;
				++epoch;
			}

			// reduce complete
			if (worker_count < 1) {
				break;
			}
			
			// idle threads
			if (w.thread_id >= worker_count) {
				continue;
			}

			const size_t offset = hist.bin_count * static_cast<size_t>(pow(2, epoch - 1));
			const int r_start = hist.bin_count * w.thread_id * static_cast<int>(pow(2, epoch));
			const int r_end = r_start + hist.bin_count - 1;

			if ((r_end + offset) >= local_bin.size()) {
				continue;
			}
			
			for (int i = r_start; i <= r_end; ++i) {
				local_bin[i] += local_bin[i + offset];
			}
		}

		// copy result to histogram
		if (w.thread_id == 0) {  // single thread region
			for (int i = 0; i < hist.bin_count; ++i) {
				hist.bin[i] = local_bin[i];
			}
		}
	};
	
	computeParallelWork(data_size, worker1);
	computeParallelWork(reduce_count, worker2, false);
}

long long benchmark(Histogram& hist, const Algorithm& algorithm) {
	cout << '[' << hist.name << "] Filling up... ";
	const auto start_point = chrono::system_clock::now();
	algorithm(hist);
	const auto end_point = chrono::system_clock::now();
	return chrono::duration_cast<chrono::seconds>(end_point - start_point).count();
}
