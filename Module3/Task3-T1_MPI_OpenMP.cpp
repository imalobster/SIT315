#include <mpi.h>
#include <iostream>
#include <cstdlib>
#include <time.h>
#include <chrono>

using namespace std::chrono;
using namespace std;

#define masterRank 0

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

void PopulateMatrix(int** matrix, int size)
{
	for (int i = 0; i < size; i++)
	{
		for (int j = 0; j < size; j++)
		{
			matrix[i][j] = rand() % 10;
		}
	}
}

void MultiplyMatrices(int** matrix1, int** matrix2, int** matrix3, int size)
{
	for (int i = 0; i < size; i++)
	{
		for (int j = 0; j < size; j++)
		{
			matrix3[i][j] = 0;
			for (int k = 0; k < size; k++)
			{
				matrix3[i][j] += matrix1[i][k] * matrix2[k][j];
			}
		}
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

	// Define list of sizes to evaluate performance with
	int sizes[] = { 10, 100, 1000 };

	// Loop through each size
	for (int size : sizes)
	{
		// Get length per available ranks
		int length = size/numtasks;

		// Generate random seed using time
		srand(time(0));
		
		// Define and allocate sub-matrices memory for all ranks
		int** m1_sub = (int**)  malloc(length * sizeof(int*));
		int** m2_sub = (int**)  malloc(length * sizeof(int*));
		int** m3_sub = (int**)  malloc(length * sizeof(int*));

		// Define main matrices for all ranks
		int **m1;
		int **m2;
		int **m3;

		// Take current time before populating
		auto startPopulate = high_resolution_clock::now();

		// Allocate and populate main matrices for head only
		if (rank == masterRank)
		{
			// Create array for each element of above arrays 
			for (int i = 0; i < size; i++)
			{
				m1[i] = (int*) malloc(length * sizeof(int));
				m2[i] = (int*) malloc(length * sizeof(int));
				m3[i] = (int*) malloc(length * sizeof(int));
			}
			// Populate first two with random variables
			PopulateMatrix(m1, size);
			PopulateMatrix(m2, size);
		}

		// Start timer for vector addition process
		auto start = high_resolution_clock::now();

		// Scatter even portion of data from v1 and v2 in head to sub vectors in all nodes
		MPI_Scatter(m1, length, MPI_INT, m1_sub, length, MPI_INT, masterRank, MPI_COMM_WORLD);
		MPI_Scatter(m2, length, MPI_INT, m2_sub, length, MPI_INT, masterRank, MPI_COMM_WORLD);

		// Multiply first two to produce third matrix
		MultiplyMatrices(m1_sub, m2_sub, m3_sub, length);

		// Gather results from all nodes and combine into v3 on head
		MPI_Gather(m3_sub, length, MPI_INT, m3, length, MPI_INT, masterRank, MPI_COMM_WORLD);

		// Calculation durations and cast to microseconds
		auto durationPopulate = duration_cast<microseconds>(startMultiply - startPopulate);
		auto durationMultiply = duration_cast<microseconds>(stop - startMultiply);

		// Print total time taken on head node
		if (rank == masterRank)
		{
			// Print equation (switched to false - only needed for verify) and time taken
			if (matrixSize <= 10)
				PrintEquation(m1, m2, m3, matrixSize, false);

			cout << "MATRIX SIZE: " << size << endl;
			cout << "Time taken to populate square matrices: " << durationPopulate.count() << " microseconds" << endl;
			cout << "Time taken to multiply square matrices: " << durationMultiply.count() << " microseconds\n" << endl;
		}
	}
	// Finalize the MPI environment
	MPI_Finalize();
}