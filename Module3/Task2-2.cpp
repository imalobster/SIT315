#include <mpi.h>
#include <iostream>
#include <cstdlib>
#include <time.h>
#include <chrono>

using namespace std::chrono;
using namespace std;

#define masterRank 0

void randomVector(int vector[], int size)
{
	for (int i = 0; i < size; i++)
	{
		// Generate a random integer between 0 and 99
		vector[i] = rand() % 100;
	}
}

int main(int argc, char** argv)
{
	// Initalise MPI variables 
	int numtasks, rank, name_len, tag=1;
	// Define char array for processor name
	char name[MPI_MAX_PROCESSOR_NAME];
	// Initialize the MPI environment
	MPI_Init(&argc,&argv);
	// Get the number of tasks/process
	MPI_Comm_size(MPI_COMM_WORLD, &numtasks);
	// Get the rank
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	// Find the processor name
	MPI_Get_processor_name(name, &name_len);

	// Define size of vectors
	unsigned long size = 10000000;
	// Get length per available ranks
	int length = size/numtasks;
	// Generate random seed using time
	srand(time(0));
	
	// Define and allocate sub-vectors memory for all ranks
	int *v1_sub = (int*)malloc(length * sizeof(int*));
	int *v2_sub = (int*)malloc(length * sizeof(int*));
	int *v3_sub = (int*)malloc(length * sizeof(int*));

	// Define main vectors for all ranks
	int *v1;
	int *v2;
	int *v3;

	// Allocate and populate main vectors for head only
	if (rank == masterRank)
	{
		v1 = (int*)malloc(length * sizeof(int*));
		v2 = (int*)malloc(length * sizeof(int*));
		v3 = (int*)malloc(length * sizeof(int*));

		randomVector(v1, size);
		randomVector(v2, size);
	}

	// Define local and total sum variables used to manage results across nodes
	int localSum = 0;
	int totalSum;

	// Start timer for vector addition process
	auto start = high_resolution_clock::now();

	// Scatter even portion of data from v1 and v2 in head to sub vectors in all nodes
	MPI_Scatter(v1, length, MPI_INT, v1_sub, length, MPI_INT, masterRank, MPI_COMM_WORLD);
	MPI_Scatter(v2, length, MPI_INT, v2_sub, length, MPI_INT, masterRank, MPI_COMM_WORLD);

	// Execute addition task on all nodes (note length has been split across nodes already)
	for (size_t i = 0; i < length; i++)
	{
		v3_sub[i] = v1_sub[i] + v2_sub[i];
		localSum += v3_sub[i];
	}

	// Gather results from all nodes and combine into v3 on head
	MPI_Gather(v3_sub, length, MPI_INT, v3, length, MPI_INT, masterRank, MPI_COMM_WORLD);

	// Retrieve finish time and calculate duration
	auto stop = high_resolution_clock::now();
	auto duration = duration_cast<microseconds>(stop - start);

	// Reduce all localSum values and sum into totalSum variable to determine total sum of all additions
	MPI_Reduce(&localSum, &totalSum, 1, MPI_INT, MPI_SUM, masterRank, MPI_COMM_WORLD);

	// Print total time taken on head node
	if (rank == masterRank)
	{
		cout << "Time taken by function: " << duration.count() << " microseconds" << endl
			<< "Total sum of vector addition: " << totalSum << endl;
	}

	// Finalize the MPI environment
	MPI_Finalize();
}