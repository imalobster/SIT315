#include <iostream>
#include <fstream>
#include <cstdlib>
#include <time.h>
#include <chrono>
#include <stdio.h>
#include <pthread.h>

using namespace std::chrono;
using namespace std;

// Define struct to hold partition details for each thread completing the matrix populating task
struct PopTask
{
	int seed;
	int **m;
	int size;
	int start;
	int end;
};

// Define struct to hold partition details for each thread completing the matrix multiplication task
struct MulTask
{
	int **m1;
	int **m2;
	int **m3;
	int size;
	int start;
	int end;
};

void PrintRow(int* array, int size)
{
	cout << "|  ";
	for (int i = 0; i < size; i++)
	{
		cout << array[i];
		if (array[i] >= 1000)
			cout << " ";
		else if (array[i] >= 100)
			cout << "  ";
		else if (array[i] >= 10)
			cout <<  "   ";
		else
			cout << "    ";
	}
	cout << "|";
}

void PrintEquation(int** matrix1, int** matrix2, int** matrix3, int size, bool print)
{
	if (!print)
		return;

	cout << endl;
	for (int i = 0; i < size; i++)
	{
		PrintRow(matrix1[i], size);
		if (i == size - 1)
			cout << "  X  ";
		else
			cout << "     ";
		PrintRow(matrix2[i], size);
		if (i == size - 1)
			cout << "  =  ";
		else
			cout << "     ";
		PrintRow(matrix3[i], size);

		cout << endl;
	}
}

void *PopulateMatrix(void *args)
{
	// Create a local variable to point to the struct passed in via the args argument
	PopTask *PopTask = ((struct PopTask *)args);
	// Initialise the pseudo-random number generator with seed value
	srand(time(NULL) * PopTask -> seed);
	// Generate random values between 0 and 99 for the partition 
	for (int i = PopTask -> start; i < PopTask -> end; i++)
	{
		for (int j = 0; j < PopTask -> size; j++)
		{
			PopTask -> m[i][j] = rand() % 10;
		}
	}
	return NULL;
}

void *MultiplyMatrices(void *args)
{
	// Create a local variable to point to the struct passed in via the args argument
	MulTask *MulTask = ((struct MulTask *)args);

	for (int i = MulTask -> start; i < MulTask -> end; i++)
	{
		for (int j = 0; j < MulTask -> size; j++)
		{
			MulTask -> m3[i][j] = 0;
			for (int k = 0; k < MulTask -> size; k++)
			{
				MulTask -> m3[i][j] += MulTask -> m1[i][k] * MulTask -> m2[k][j];
			}
		}
	}
	return NULL;
}

void p_PopulateMatrix(int** matrix, int seed, int totalSize, int partitionSize, int threads, pthread_t arr[])
{
	for (size_t i = 0; i < (threads / 2); i++)
	{
		// Create new random task struct for each thread to store respective partition data
		struct PopTask *PopTask = (struct PopTask *)malloc(sizeof(struct PopTask));
		// Assign the vector pointer of the struct to the specified vector that needs to be populated
		PopTask -> m = matrix;
		// Specify the seed value and increment it for each thread to generate random values
		PopTask -> seed = seed++;
		// Set the size of the matrix
		PopTask -> size = totalSize;
		// Set the start index of the partition the thread will begin generating numbers for
		PopTask -> start = i * partitionSize;
		// Set the end index of the partition for the thread, ensuring if it's the last row it returns the size of the full matrix size
		PopTask -> end = (i + 1) == (threads / 2) ? totalSize : ((i + 1) * partitionSize);
		// Create the thread and ask it to compute the PopulateMatrix function using the struct prepared above
		pthread_create(&arr[i], NULL, PopulateMatrix, (void *)PopTask);
	}
}

void p_MultiplyMatrices(int** matrix1, int** matrix2, int** matrix3, int totalSize, int partitionSize, int threads, pthread_t arr[])
{
	for (size_t i = 0; i < threads; i++)
	{
		// Create new random task struct for each thread to store respective partition data
		struct MulTask *MulTask = (struct MulTask *)malloc(sizeof(struct MulTask));
		// Assign the first vector pointer of the struct
		MulTask -> m1 = matrix1;
		// Assign the second vector pointer of the struct
		MulTask -> m2 = matrix2;
		// Assign the thrid vector pointer of the struct (this vector will be the resultant vector)
		MulTask -> m3 = matrix3;
		// Set the size of the matrix
		MulTask -> size = totalSize;
		// Set the start index to the beginning of the partition
		MulTask -> start = i * partitionSize;
		// Set the end index of the partition for the thread, ensuring if it's the last row it returns the size of the full matrix size
		MulTask -> end = (i + 1) == threads ? totalSize : ((i + 1) * partitionSize);
		// Create the thread and ask it to calculate the product of m1 and m2 for m3
		pthread_create(&arr[i], NULL, MultiplyMatrices, (void *)MulTask);
	}
}

int main()
{
	// Delete any existing results file
	remove("results_pthread.txt");

	// Define sizes of matrices
	int n_sizes[] = { 10, 100, 1000 };

	// Different varying thread counts
	int n_threads[] = { 2, 8, 16, 24 };

	for (int size : n_sizes)
	{
		for (int threads : n_threads)
		{
			// Define matrix size
			int matrixSize = size;

			// Define thread count
			int numThreads = threads;

			// Initisalise number generator
			srand(time(0));

			// Declare empty 2D arrays (matrices)
			int** m1 = (int**)  malloc(matrixSize * sizeof(int*));
			int** m2 = (int**)  malloc(matrixSize * sizeof(int*));
			int** m3 = (int**)  malloc(matrixSize * sizeof(int*));

			// Create array for each element of above arrays 
			for (int i = 0; i < matrixSize; i++)
			{
				m1[i] = (int*) malloc(matrixSize * sizeof(int));
				m2[i] = (int*) malloc(matrixSize * sizeof(int));
				m3[i] = (int*) malloc(matrixSize * sizeof(int));
			}

			// Create thread array for populating matrices
			pthread_t threads_popTask[numThreads];
			// Create thread array for matrix multiplication
			pthread_t threads_mulTask[numThreads];

			// Take current time before populating
			auto startPopulate = high_resolution_clock::now();

			// Calculate the partition size per thread needed for matrix population task (note divide by two since we want to split threads between m1 and m2)
			int partitionSize = matrixSize / (numThreads / 2);

			// Populate first two with random variables
			p_PopulateMatrix(m1, 0, matrixSize, partitionSize, numThreads, threads_popTask);
			p_PopulateMatrix(m2, threads, matrixSize, partitionSize, numThreads, threads_popTask);

			// Wait for all threads created above to complete
			for (size_t i = 0; i < numThreads; i++)
			{
				pthread_join(threads_popTask[i], NULL);
			}
			
			// Take current time before multiplication
			auto startMultiply = high_resolution_clock::now();

			// Calculate the partition size per thread needed for matrix multiplication task
			partitionSize = matrixSize / numThreads;

			// Multiply first two to produce third matrix
			p_MultiplyMatrices(m1, m2, m3, matrixSize, partitionSize, numThreads, threads_mulTask);

			// Wait for all threads created above to complete
			for (size_t i = 0; i < numThreads; i++)
			{
				pthread_join(threads_mulTask[i], NULL);
			}

			// Take current time before multiplication
			auto stop = high_resolution_clock::now();

			// Calculation durations and cast to microseconds
			auto durationPopulate = duration_cast<microseconds>(startMultiply - startPopulate);
			auto durationMultiply = duration_cast<microseconds>(stop - startMultiply);

			// Print equation (switched to false - only needed for verify) and time taken
			if (matrixSize <= 10)
				PrintEquation(m1, m2, m3, matrixSize, false);
			cout << "MATRIX SIZE: " << size << ", THREADS: " << numThreads << endl;
			cout << "Time taken to populate square matrices: " << durationPopulate.count() << " microseconds" << endl;
			cout << "Time taken to multiply square matrices: " << durationMultiply.count() << " microseconds\n" << endl;

			// Redirect stdout to file and call above again
			freopen("results_pthread.txt", "a", stdout);
			cout << "MATRIX SIZE: " << size << ", THREADS: " << numThreads << endl;
			cout << "Time taken to populate square matrices: " << durationPopulate.count() << " microseconds" << endl;
			cout << "Time taken to multiply square matrices: " << durationMultiply.count() << " microseconds\n" << endl;

			// Redirect stdout to terminal again
			freopen("CON", "w", stdout);
		}
	}
	return 0;
}