#include <iostream>
#include <cstdlib>
#include <time.h>
#include <chrono>

using namespace std::chrono;
using namespace std;

void randomVector(int vector[], int size)
{
	for (int i = 0; i < size; i++)
	{
		// Generate a random integer between 0 and 99
		vector[i] = rand() % 100;
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

	// Use the custom function to generate random values in the first and second blocks of memory
	randomVector(v1, size);
	randomVector(v2, size);

	// Assign values to the third block of memory by adding the values of the respective index location from the first and second blocks
	for (int i = 0; i < size; i++)
	{
		v3[i] = v1[i] + v2[i];
	}

	// Get the current time after vector assignment
	auto stop = high_resolution_clock::now();

	// Obtain the difference between start and stop times, then cast to microseconds format
	auto duration = duration_cast<microseconds>(stop - start);

	cout << "Time taken by function: "
		 << duration.count() << " microseconds" << endl;

	return 0;
}