#include <iostream>
#include "util.h"

template <typename T>
void readNumber(T& number, string label, T min, T max) {
	string input;
	cout << label << ": ";
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
		readNumber(number, label, min, max);
	}
	catch (out_of_range) {
		cout << "Number out of range." << endl;
		readNumber(number, label, min, max);
	}
}

template void readNumber<int>(int& number, string label, int min, int max);

template void readNumber<float>(float& number, string label, float min, float max);
