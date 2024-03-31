#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <functional>

using namespace std;

class Histogram {
public:
	string name;
	vector<float>& data;
	vector<int> bin;
	float bin_size;
	int bin_count;

	Histogram(string name, vector<float>& data, float bin_size, int bin_count):
		name(name),
		data(data),
		bin(bin_count, 0),
		bin_size(bin_size),
		bin_count(bin_count)
	{}
};

typedef function<void(Histogram&)> Algorithm;

class Work {
public:
	const int thread_id;
	const int num_threads;
	int& progress;
	function<int()> get_total_progress;

	Work(int thread_id, int num_threads, int& progress, function<int()> get_progress):
		thread_id(thread_id),
		num_threads(num_threads),
		progress(progress),
		get_total_progress(std::move(get_progress))
	{}
};

inline int getReduceCount(int init_size);

inline int getWorkerCount();

inline float getRandomFloat();

vector<float> generateNumbers(int count);

void computeParallelWork(int target, const function<void(Work&)>& worker, bool track = true);

void serialized(Histogram& hist);

void synchronized(Histogram& hist);

void localBin(Histogram& hist);

void reduction(Histogram& hist);

long long benchmark(Histogram& hist, const Algorithm& algorithm);
