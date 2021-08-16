#include <iostream>
#include <cstdlib>
#include <time.h>
#include <chrono>
#include <omp.h>

using namespace std::chrono;
using namespace std;

const int NUM_THREADS = 24;

void randomVector(int vector[], int size)
{
	#pragma omp parallel
	{
		// Generate random values between 0 and 99 for the partition 
		#pragma omp for
		for (int i = 0; i < size; i++)
		{
			vector[i] = rand() % 100;
		}
	}
}

int main(){

	unsigned long size = 100000000;

	srand(time(0));

	int *v1, *v2, *v3;

	// Get the current time before vector assignment
	auto start = high_resolution_clock::now();

	// Allocate three lots of contiguous memory of a set size and return the pointer to the first location
	v1 = (int *) malloc(size * sizeof(int *));
	v2 = (int *) malloc(size * sizeof(int *));
	v3 = (int *) malloc(size * sizeof(int *));

	omp_set_num_threads(NUM_THREADS);

	// Use the custom function to generate random values in the first and second blocks of memory
	randomVector(v1, size);
	randomVector(v2, size);
	
	#pragma omp parallel
	{
		// Assign values to v3 based on the sum of values from v1 and v2
		#pragma omp for
		for (int i = 0; i < size; i++)
		{
			v3[i] = v1[i] + v2[i];
		}
	}

	// Get the current time after vector assignment
	auto stop = high_resolution_clock::now();

	// Obtain the difference between start and stop times, then cast to microseconds format
	auto duration = duration_cast<microseconds>(stop - start);

	cout << "Time taken by function: "
		 << duration.count() << " microseconds" << endl;

	return 0;
}