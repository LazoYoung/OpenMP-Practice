#pragma once
#include <string>
#include <climits>

using namespace std;

template<typename T>
void readNumber(T& number, string label, T min = numeric_limits<T>::min(), T max = numeric_limits<T>::max());
