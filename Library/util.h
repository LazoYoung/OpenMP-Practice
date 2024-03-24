#pragma once
#include <string>
#include <climits>

using namespace std;

template<typename T>
void readNumber(T& number, string prompt, T min = numeric_limits<T>::lowest(), T max = numeric_limits<T>::max());
