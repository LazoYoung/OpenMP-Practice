#include <iostream>
#include <vector>
#include "util.h"

int numberOfDigits(int number) {
	int digits = (number == 0) ? 1 : 0;

	while (number) {
		number /= 10;
		digits++;
	}
	return digits;
}

template <typename T>
void print(vector<T>& vec, int trimCount) {
	for (int i = 0; i < vec.size(); ++i) {
		if (i >= trimCount) {
			cout << "...and " << vec.size() - i << " more";
			break;
		}

		cout << vec[i] << ' ';
	}

	cout << endl;
}

template <typename T>
void readNumber(T& number, string prompt, T min, T max) {
	string input;
	cout << prompt << ": ";
	getline(cin, input);

	try {
		T num;

		if constexpr (is_floating_point_v<T>) {
			num = stof(input);
		}
		else {
			num = stoi(input);
		}

		if (num < min || num > max) {
			throw out_of_range("Number out of range.");
		}
		else {
			number = num;
		}
	}
	catch (invalid_argument) {
		cout << "Not a valid number." << endl;
		readNumber(number, prompt, min, max);
	}
	catch (out_of_range) {
		cout << "Number out of range." << endl;
		readNumber(number, prompt, min, max);
	}
}

template void print(vector<int>& vec, int trimCount);

template void print(vector<float>& vec, int trimCount);

template void print(vector<double>& vec, int trimCount);

template void readNumber<int>(int& number, string prompt, int min, int max);

template void readNumber<float>(float& number, string prompt, float min, float max);

template void readNumber<double>(double& number, string prompt, double min, double max);
