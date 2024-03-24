#pragma once
#include <functional>

using namespace std;

struct INTEGRAL {
	double a;
	double b;
	int n;
	function<double(double)> fn;
};

void compute(INTEGRAL data, bool parallel);

double integral(INTEGRAL data, bool parallel);

function<double(double)> readFunction();