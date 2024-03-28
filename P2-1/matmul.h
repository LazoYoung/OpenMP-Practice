#pragma once
#include <vector>
#include <string>

using namespace std;

size_t index(int r, int c, int cols);

float getRandomFloat();

vector<float> getRandomMatrix(int rows, int cols);

vector<float> getRandomVector(int length);

vector<float> matmul(vector<float>& A, vector<float>& b, int rows, int cols, int threads);

void compute(vector<float>& A, vector<float>& b, int rows, int cols, int threads);
