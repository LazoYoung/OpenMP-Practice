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

int main() {
	omp_set_nested(1);

	Algorithm algorithm[_VERSIONS] = {
		serialized, synchronized, localBin, reduction
	};
	string hist_name[_VERSIONS] = {
		"v0_serial", "v1_sync", "v2_localBin", "v3_reduce"
	};

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

inline float getRandomFloat() {
	static int precision = 1000;
	static int div = _R_RANGE * precision;
	return (float)(rand() % div) / precision + _R_MIN;
}

vector<float> generateNumbers(int count) {
	vector<float> data(count);
	int progress = 0;
	auto worker = [&](Work& work) {
		int tid = work.thread_id;
		int threads = work.num_threads;
		int workload = count / threads;
		int start = tid * workload;
		int end = (tid < threads - 1) ? (tid + 1) * workload : count;

		for (int i = start; i < end; ++i) {
			data[i] = getRandomFloat();

			#pragma omp atomic
			++progress;
		}
	};

	cout << "Generating numbers... ";
	computeParallelWork(progress, count, worker);
	return data;
}

void computeParallelWork(int& progress, int target, function<void(Work&)> worker) {
	#pragma omp parallel
	{
		int num_threads = omp_get_num_threads() - 1;

		if (omp_get_thread_num() == num_threads) {
			int last_width = 0;

			while (true) {
				int _progress;

				#pragma omp critical
				{
					_progress = progress;
				}

				if (last_width > 0) {
					cout << string(last_width, '\b') << string(last_width, ' ') << string(last_width, '\b');
				}

				int percent = (int)round((double)_progress / target * 100);
				int width = numberOfDigits(_progress) + numberOfDigits(percent) + 4;
				last_width = width;

				cout << _progress << " (" << percent << "%)";

				if (_progress < target) {
					this_thread::sleep_for(chrono::milliseconds(500));
				}
				else {
					cout << endl;
					break;
				}
			}
		}
		else {
			Work work;
			work.thread_id = omp_get_thread_num();
			work.num_threads = num_threads;

			worker(work);
		}
	}
}

void serialized(Histogram& hist) {
	int progress = 0;
	int dataSize = (int)hist.data.size();
	auto worker = [&](Work& work) {
		if (work.thread_id != 0) {
			// effectively makes it serially processed
			return;
		}

		for (int i = 0; i < dataSize; ++i) {
			float number = hist.data[i];
			int idx = (int)floor(number / hist.bin_size);
			idx = (idx < hist.bin_count) ? idx : hist.bin_count - 1;

			++hist.bin[idx];
			++progress;
		}
	};

	computeParallelWork(progress, dataSize, worker);
}

void synchronized(Histogram& hist) {
	int progress = 0;
	int dataSize = (int)hist.data.size();
	auto worker = [&](Work& work) {
		int num_threads = work.num_threads;
		int workload = dataSize / num_threads;
		int tid = work.thread_id;
		int start = tid * workload;
		int end = (tid < num_threads - 1) ? (tid + 1) * workload : dataSize;

		for (int i = start; i < end; ++i) {
			float number = hist.data[i];
			int idx = (int)floor(number / hist.bin_size);
			idx = (idx < hist.bin_count) ? idx : hist.bin_count - 1;

			#pragma omp atomic
			++hist.bin[idx];

			#pragma omp atomic
			++progress;
		}
	};
	
	computeParallelWork(progress, dataSize, worker);
}

void localBin(Histogram& hist) {
	// todo method stub
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
