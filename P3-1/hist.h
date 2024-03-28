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

	Histogram(string name, vector<float>& data, float bin_size, int bin_count) :
		name(name),
		data(data),
		bin(bin_count, 0),
		bin_size(bin_size),
		bin_count(bin_count)
	{}
};

typedef function<void(Histogram&)> Algorithm;

struct Work {
	int thread_id;
	int num_threads;
};

inline float getRandomFloat();

vector<float> generateNumbers(int count);

void computeParallelWork(int& progress, int target, function<void(Work&)> worker);

void serialized(Histogram& hist);

void synchronized(Histogram& hist);

void localBin(Histogram& hist);

void reduction(Histogram& hist);

long long benchmark(Histogram& hist, Algorithm algorithm);
