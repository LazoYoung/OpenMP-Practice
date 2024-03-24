#include <iostream>
#include <vector>
#include <chrono>
#include <functional>
#include <cmath>
#include "omp.h"
#include "util.h"
#include "integral.h"

int main() {
	INTEGRAL data;
	data.fn = readFunction();
	readNumber(data.a, "Bound a");
	readNumber(data.b, "Bound b");
	readNumber(data.n, "Resolution n", 1);

	cout << endl << "[SERIAL] Computing integral..." << endl;
	compute(data, false);

	cout << endl << "[PARALLEL] Computing integral..." << endl;
	compute(data, true);
}

void compute(INTEGRAL data, bool parallel) {
	auto startTime = chrono::system_clock::now();
	double area = integral(data, parallel);
	auto endTime = chrono::system_clock::now();
	auto period = chrono::duration_cast<chrono::milliseconds>(endTime - startTime);

	cout << "Area: " << area << endl;
	cout << "Elapsed time: " << period.count() << " ms" << endl;
}

double integral(INTEGRAL data, bool parallel) {
	double area = 0.0f;
	double a = data.a;
	double b = data.b;
	int n = data.n;
	double delta = (b - a) / n;
	auto& fn = data.fn;
	vector<double> rect_area(n);

#pragma omp parallel if(parallel)
	{
		if (omp_in_parallel()) {
#pragma omp single
			cout << "# of threads: " << omp_get_num_threads() << endl;
		}

#pragma omp for
		for (int i = 0; i < n; ++i) {
			double x = a + i * delta;
			double y = (fn(x) + fn(x + delta)) / 2;
			rect_area[i] = delta * y;
		}

#pragma omp single
		for (int i = 0; i < n; ++i) {
			area += rect_area[i];
		}
	}

	return area;
}

function<double(double)> readFunction() {
	function<double(double)> fn;
	int mode;

	cout << "[1] parabola   : y = x^2" << endl;
	cout << "[2] sine       : y = sin(x)" << endl;
	cout << "[3] polynomial : y = x^3 - 3x^2 + 5x + 1 " << endl;
	readNumber(mode, "Select a function...", 1, 3);

	switch (mode) {
	case 1:
		fn = [](double x) { return (x * x); };
		break;
	case 2:
		fn = [](double x) { return sin(x); };
		break;
	case 3:
		fn = [](double x) { return pow(x, 3) - 3 * pow(x, 2) + 5 * x + 1; };
		break;
	}

	return fn;
}
