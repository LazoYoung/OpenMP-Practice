#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
#include <random>
#include <functional>
#include <cmath>
#include <cstdio>
#include "omp.h"
#include "util.h"
#include "hist.h"

using namespace std;

const int _DATA_COUNT = 1024 * 1024 * 1024;
const int _VERSIONS = 4;
const int _R_MIN = 0;
const int _R_MAX = 10;
const int _R_RANGE = _R_MAX - _R_MIN;
const Algorithm algorithm[_VERSIONS] = {
		serialized, synchronized, localBin, reduction
};
const string hist_name[_VERSIONS] = {
	"v0_serial", "v1_sync", "v2_localBin", "v3_reduce"
};

int main() {
	float bin_size;
	readNumber(bin_size, "Size of each bin", 0.00001f, (float)_R_MAX);
	int bin_count = (int)ceil(_R_RANGE / bin_size);
	vector<float> data = generateNumbers(_DATA_COUNT);

	cout << "Data: ";
	print(data, 10);
	cout << endl;

	for (int i = 0; i < _VERSIONS; ++i) {
		Histogram hist(hist_name[i], data, bin_size, bin_count);
		auto seconds = benchmark(hist, algorithm[i]);
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
	static int precision = 1000;
	static int div = _R_RANGE * precision;
	return (float)(rand() % div) / precision + _R_MIN;
}

vector<float> generateNumbers(int count) {
	vector<float> data(count);
	auto worker = [&](Work& work) {
		int tid = work.thread_id;
		int threads = work.num_threads;
		int workload = count / threads;
		int start = tid * workload;
		int end = (tid < threads - 1) ? (tid + 1) * workload : count;

		for (int i = start; i < end; ++i) {
			data[i] = getRandomFloat();
			++work.progress;
		}
	};

	cout << "Generating numbers... ";
	computeParallelWork(count, worker);
	return data;
}

void computeParallelWork(int target, function<void(Work&)> worker) {
	int worker_count = getWorkerCount();
	vector<int> progress_bin(worker_count, 0);

	#pragma omp parallel num_threads(worker_count + 1)
	{
		if (omp_get_thread_num() == worker_count) {
			int last_width = 0;

			while (true) {
				int progress = 0;

				for (int i = 0; i < worker_count; ++i) {
					progress += progress_bin[i];
				}

				if (last_width > 0) {
					cout << string(last_width, '\b') << string(last_width, ' ') << string(last_width, '\b');
				}

				int percent = (int)round((double)progress / target * 100);
				int width = numberOfDigits(progress) + numberOfDigits(percent) + 4;
				last_width = width;

				cout << progress << " (" << percent << "%)";

				if (progress < target) {
					this_thread::sleep_for(chrono::milliseconds(500));
				}
				else {
					cout << endl;
					break;
				}
			}
		}
		else {
			int id = omp_get_thread_num();
			Work work(id, worker_count, progress_bin[id]);

			worker(work);
		}
	}
}

void serialized(Histogram& hist) {
	int data_size = (int)hist.data.size();
	auto worker = [&](Work& work) {
		if (work.thread_id != 0) {
			// effectively makes it serially processed
			return;
		}

		for (int i = 0; i < data_size; ++i) {
			float number = hist.data[i];
			int idx = (int)floor(number / hist.bin_size);
			idx = (idx < hist.bin_count) ? idx : hist.bin_count - 1;

			++hist.bin[idx];
			++work.progress;
		}
	};

	computeParallelWork(data_size, worker);
}

void synchronized(Histogram& hist) {
	int data_size = (int)hist.data.size();
	auto worker = [&](Work& work) {
		int num_threads = work.num_threads;
		int workload = data_size / num_threads;
		int tid = work.thread_id;
		int start = tid * workload;
		int end = (tid < num_threads - 1) ? (tid + 1) * workload : data_size;

		for (int i = start; i < end; ++i) {
			float number = hist.data[i];
			int idx = (int)floor(number / hist.bin_size);
			idx = (idx < hist.bin_count) ? idx : hist.bin_count - 1;

			#pragma omp atomic
			++hist.bin[idx];

			++work.progress;
		}
	};
	
	computeParallelWork(data_size, worker);
}

void localBin(Histogram& hist) {
	int data_size = (int)hist.data.size();
	int worker_count = getWorkerCount();
	vector<int> local_bin(worker_count, 0);
	auto worker = [&](Work& work) {
		
	};

	computeParallelWork(data_size, worker);
}

void reduction(Histogram& hist) {
	// todo method stub
}

long long benchmark(Histogram& hist, Algorithm algorithm) {
	cout << '[' << hist.name << "] Filling up... ";
	auto startPoint = chrono::system_clock::now();
	algorithm(hist);
	auto latency = chrono::duration_cast<chrono::seconds>(chrono::system_clock::now() - startPoint);
	return latency.count();
}
