#pragma once
#include <string>
#include <climits>

using namespace std;

int numberOfDigits(int number);

template <typename T>
void print(vector<T>& vec, size_t trim_count = 10);

template<typename T>
void readNumber(T& number, string prompt, T min = numeric_limits<T>::lowest(), T max = numeric_limits<T>::max());
