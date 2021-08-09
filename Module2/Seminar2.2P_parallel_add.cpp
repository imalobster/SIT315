#include <iostream>
#include <cstdlib>
#include <time.h>
#include <chrono>
#include <pthread.h>

using namespace std::chrono;
using namespace std;

#define NUM_THREADS 8

// Define struct to hold partition details for each thread completing the vector addition task
struct AddTask
{
	int *v1, *v2, *v3;
	int start;
	int end;
};

void randomVector(int vector[], int size)
{
	for (int i = 0; i < size; i++)
	{
		// Generate a random integer between 0 and 99
		vector[i] = rand() % 100;
	}
}

// Function that generates and assigns numbers to an array (passed via RngTask struct object)
void *addVector(void *args)
{
	// Create a local variable to point to the struct passed in via the args argument
	AddTask *AddTask = ((struct AddTask *)args);

	// Assign values to v3 by adding the v1 and v2 values at each respective index
	for (int i = AddTask -> start; i < AddTask -> end; i++)
	{
		AddTask -> v3[i] = AddTask -> v1[i] + AddTask -> v2[i];
	}
	return NULL;
}

// Handles the creation of each thread used in the vector addition task
void p_addVectorByIndex(int* vector1, int* vector2, int* vector3, int totalSize, int partitionSize, pthread_t arr[])
{
	for (size_t i = 0; i < NUM_THREADS; i++)
	{
		// Create new random task struct for each thread to store respective partition data
		struct AddTask *AddTask = (struct AddTask *)malloc(sizeof(struct AddTask));
		// Assign the first vector pointer of the struct
		AddTask -> v1 = vector1;
		// Assign the second vector pointer of the struct
		AddTask -> v2 = vector2;
		// Assign the thrid vector pointer of the struct (this vector will be the resultant vector)
		AddTask -> v3 = vector3;
		// Set the start index to the beginning of the partition
		AddTask -> start = i * partitionSize;
		// Set the end index to the end of the partition for the thread, ensuring if it's the last vector it returns the size of the full array
		AddTask -> end = (i + 1) == NUM_THREADS ? totalSize : ((i + 1) * partitionSize);
		// Create the thread and ask it to add values from v1 and v2 into v3
		pthread_create(&arr[i], NULL, addVector, (void *)AddTask);
	}

	// Wait for all threads created above to complete
	for (size_t i = 0; i < NUM_THREADS; i++)
	{
		pthread_join(arr[i], NULL);
	}
}

int main(){
	unsigned long size = 100000000;

	srand(time(0));

	int *v1, *v2, *v3;

	// Create thread array for the addition task
	pthread_t threads_addTask[NUM_THREADS];

	// Get the current time before vector assignment
	auto start = high_resolution_clock::now();

	// Allocate three lots of contiguous memory of a set size and return the pointer to the first location
	v1 = (int *) malloc(size * sizeof(int *));
	v2 = (int *) malloc(size * sizeof(int *));
	v3 = (int *) malloc(size * sizeof(int *));

	// Use the custom function to generate random values in the first and second blocks of memory
	randomVector(v1, size);
	randomVector(v2, size);

	// Calculate the partition size per thread needed for vector addition task
	int partitionSize = size / NUM_THREADS;

	// Assign values to the third block of memory by adding the values of the respective index location from the first and second blocks
	p_addVectorByIndex(v1, v2, v3, size, partitionSize, threads_addTask);

	// Get the current time after vector assignment
	auto stop = high_resolution_clock::now();

	// Obtain the difference between start and stop times, then cast to microseconds format
	auto duration = duration_cast<microseconds>(stop - start);

	cout << "Time taken by function: "
		 << duration.count() << " microseconds" << endl;

	return 0;
}