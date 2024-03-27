#include <iostream>
#include <chrono>
#include <random>
#include "omp.h"
#include "util.h"
#include "matmul.h"

int main() {
	int rows, cols, threads;
	readNumber(rows, "Number of rows", 1);
	readNumber(cols, "Number of columns", 1);
	readNumber(threads, "Number of threads", 2);
	vector<float> A1 = getRandomMatrix(rows, cols);
	vector<float> b1 = getRandomVector(cols);
	vector<float> A2(A1);
	vector<float> b2(b1);

	cout << endl << "[SERIAL] Computing X = A * b ..." << endl;
	compute(A1, b1, rows, cols, 1);
	cout << endl << "[PARALLEL] Computing X = A * b ..." << endl;
	compute(A2, b2, rows, cols, threads);
	return EXIT_SUCCESS;
}

void compute(vector<float>& A, vector<float>& b, int rows, int cols, int threads) {
	cout << "Tensor A (" << rows << 'x' << cols << "): ";
	print(A);
	cout << "Tensor b (" << cols << "x1): ";
	print(b);
	cout << "Tensor X (" << rows << "x1): ";

	auto startTime = chrono::system_clock::now();
	vector<float> matrix = matmul(A, b, rows, cols, threads);
	auto endTime = chrono::system_clock::now();
	auto period = chrono::duration_cast<chrono::milliseconds>(endTime - startTime);

	print(matrix);
	cout << "Elapsed time: " << period.count() << " ms" << endl;
}

vector<float> matmul(vector<float>& A, vector<float>& b, int rows, int cols, int threads) {
	vector<float> mat(rows);

#pragma omp parallel num_threads(threads)
	{
#pragma omp for
		for (int r = 0; r < rows; ++r) {
			float sum = 0;

			for (int i = 0; i < cols; ++i) {
				sum += A[index(r, i, cols)] * b[i];
			}

			mat[r] = sum;
		}
	}

	return mat;
}

size_t index(int r, int c, int cols) {
	return r * cols + c;
}

float getRandomFloat() {
	static const float MAX_FLOAT = 100.0f;
	static random_device rd;
	static mt19937 gen(rd());
	static uniform_real_distribution<float> random(-MAX_FLOAT, MAX_FLOAT);
	return random(gen);
}

vector<float> getRandomMatrix(int rows, int cols) {
	size_t size = rows * cols;
	vector<float> mat(rows * cols);

	for (int i = 0; i < size; ++i) {
		mat[i] = getRandomFloat();
	}
	return mat;
}

vector<float> getRandomVector(int length) {
	vector<float> vec(length);

	for (int i = 0; i < length; ++i) {
		vec[i] = getRandomFloat();
	}
	return vec;
}
