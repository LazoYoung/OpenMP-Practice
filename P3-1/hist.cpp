#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
#include <random>
#include <functional>
#include <cmath>
#include <cstdio>
#include "util.h"
#include "omp.h"

using namespace std;

const int _R_MIN = 0;
const int _R_MAX = 10;
const int _R_RANGE = _R_MAX - _R_MIN;

struct WORKER {
	int thread_id;
	int num_threads;
};

float getRandomUniformFloat() {
	static random_device rd;
	static mt19937 gen(rd());
	static uniform_real_distribution<float> random(_R_MIN, _R_MAX);
	return random(gen);
}

inline float getRandomFloat() {
	static int precision = 1000;
	static int div = _R_RANGE * precision;
	return (float)(rand() % div) / precision + _R_MIN;
}

void computeParallelWork(int& progress, int target, string message, function<void(WORKER)> worker) {
	cout << message;

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
			WORKER arg;
			arg.thread_id = omp_get_thread_num();
			arg.num_threads = num_threads;

			worker(arg);
		}
	}
}

void generateNumbers(vector<float>& data) {
	int progress = 0;
	int dataSize = (int) data.size();
	auto worker = [&](WORKER worker) {
		int tid = worker.thread_id;
		int threads = worker.num_threads;
		int workload = dataSize / threads;
		int start = tid * workload;
		int end = (tid < threads - 1) ? (tid + 1) * workload : dataSize;

		for (int i = start; i < end; ++i) {
			data[i] = getRandomFloat();

			#pragma omp atomic
			++progress;
		}
	};

	computeParallelWork(progress, dataSize, "Generating numbers... ", worker);
}

void synchronize(vector<float>& data, vector<int>& bin, float binSize, int binCount) {
	int progress = 0;
	int dataSize = (int)data.size();
	auto worker = [&](WORKER worker) {
		int num_threads = worker.num_threads;
		int workload = dataSize / num_threads;
		int tid = worker.thread_id;
		int start = tid * workload;
		int end = (tid < num_threads - 1) ? (tid + 1) * workload : dataSize;

		for (int i = start; i < end; ++i) {
			float number = data[i];
			int idx = (int)floor(number / binSize);
			idx = (idx < binCount) ? idx : binCount - 1;

			#pragma omp atomic
			++bin[idx];

			#pragma omp atomic
			++progress;
		}
	};
	
	computeParallelWork(progress, dataSize, "[v1_sync] Populating bins... ", worker);
}

int main() {
	vector<float> data(1024 * 1024 * 1024);
	vector<int> bin;
	float bin_size;
	int bin_count;

	omp_set_nested(1);
	readNumber(bin_size, "Size of each bin", 0.00001f, (float) _R_MAX);
	bin_count = (int) ceil(_R_RANGE / bin_size);
	bin = vector<int>(bin_count);

	auto startPoint = chrono::system_clock::now();
	generateNumbers(data);
	synchronize(data, bin, bin_size, bin_count);
	auto duration = chrono::duration_cast<chrono::seconds>(chrono::system_clock::now() - startPoint);

	cout << "Bins: ";
	print(bin, 10);

	cout << "Data: ";
	print(data, 10);

	cout << "Elapsed time: " << duration.count() << "s" << endl;
}
