#include <iostream>
#include <cstdlib>
#include <vector>
#include <random>
#include <chrono>
#include <stdio.h>
#include <omp.h>
using namespace std;

const int ROWS = 4096;
const int COLS = 4096;

size_t index(int row, int col) {
	return row * COLS + col;
}

void vector_sum(int thread_count, bool parallel) {
	random_device rd;
	mt19937 gen(rd());
	uniform_int_distribution<int> random(0, 50);
	vector<int> vec_A(ROWS * COLS);
	vector<int> vec_B(ROWS * COLS);
	vector<int> vec_C(ROWS * COLS);

	auto startTime = chrono::system_clock::now();

#pragma omp parallel for num_threads(thread_count) if(parallel)
	for (int i = 0; i < ROWS; ++i) {
		for (int j = 0; j < COLS; ++j) {
			size_t idx = index(i, j);
			vec_A[idx] = random(gen);
			vec_B[idx] = random(gen);
		}
	}

#pragma omp parallel for num_threads(thread_count) if(parallel)
	for (int i = 0; i < ROWS; ++i) {
		for (int j = 0; j < COLS; ++j) {
			size_t idx = index(i, j);
			vec_C[idx] = vec_A[idx] + vec_B[idx];
		}
	}

	auto endTime = chrono::system_clock::now();
	auto ms = chrono::duration_cast<chrono::milliseconds>(endTime - startTime);
	cout << "Elapsed time: " << ms.count() << " ms" << endl;
}

int main(int argc, char* argv[]) {
	vector_sum(atoi(argv[1]), true);
	return 0;
}
