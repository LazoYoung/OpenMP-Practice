#include <stdio.h>
#include <cstdlib>
#include <omp.h>

int main(int argc, char* argv[]) {
	int thread_count = std::atoi(argv[1]);

#pragma omp parallel num_threads(thread_count)
	{
		printf("[Thread %d/%d] Hello OpenMP!\n", omp_get_thread_num(), omp_get_num_threads());
	}

	return 0;
}