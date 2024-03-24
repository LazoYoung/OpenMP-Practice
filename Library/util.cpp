#include <iostream>
#include "util.h"

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

template void readNumber<int>(int& number, string prompt, int min, int max);

template void readNumber<float>(float& number, string prompt, float min, float max);

template void readNumber<double>(double& number, string prompt, double min, double max);
