#include <mpi.h>
#include <iostream>
#include <cstdlib>
#include <time.h>
#include <chrono>

using namespace std::chrono;
using namespace std;

// Set rank of master node to 0
#define masterRank 0

// This function prints an individual row of a matrix using appropriate spacing
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

// This function prints both input matrices and the output matrix in the form of an equation (should only be called if size < 10 due to formatting issues)
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
	cout << endl;
}

// This function allocates contiguous memory for a single square matrix based on its rows and cols
void InitialiseMatrix(int** &matrix, int rows, int cols, int size)
{
	matrix = (int**)malloc(rows * cols * sizeof(int*));
	int *tempRow = (int*)malloc(rows * cols * sizeof(int));

	for (int i = 0; i < size; i++)
	{
		matrix[i] = &tempRow[i * cols];
	}
}

// This function populates an input matrix with random integers less than 10
void PopulateMatrix(int** &matrix, int rows, int cols)
{
	for (int i = 0; i < rows; i++)
	{
		for (int j = 0; j < cols; j++)
		{
			matrix[i][j] = rand() % 10;
		}
	}
}

// This function multiplies two matrices together and stores the output in a third
void MultiplyMatrices(int** matrix1, int** matrix2, int** matrix3, int rows, int size)
{
	for (int i = 0; i < rows; i++)
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

// Main execution function that manages the sequence of execution
int main(int argc, char** argv)
{
	// Initalise MPI variables 
	int numtasks, rank;
	// Initialize the MPI environment
	MPI_Init(&argc, &argv);
	// Get the number of tasks/process
	MPI_Comm_size(MPI_COMM_WORLD, &numtasks);
	// Get the rank
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    // Define sizes of matrices
    int n_sizes[] = { 1, 10, 50, 100, 500, 1000 };

    for (int size : n_sizes)
    {
        // Generate random seed using time
        srand(time(0)); 
        
        // Get the count of data to be broadcasted for a single matrix
        int broadcast_size = size * size;
        // Calculate how many rows to send to each node
        int scatter_rows = size / numtasks;
        // Create an array to store how much data to send across to each node
        int sendcounts[numtasks];
        // Create an array to store the displacement values used for keeping track of data sent for each node
        int displs[numtasks];
        // Increment variable to store the most recent displacement value
        int increment = 0;

        // Create two sub-matrices for internal processing of each node (m2_sub not needed since m2 will be broadcasted)
        int **m1_sub;
        int **m3_sub;

        // Create main matrices for the two inputs and outputs
        int **m1;
        int **m2;
        int **m3;

        // Take current time before executing multiplcation
        auto start = high_resolution_clock::now();

        // If running on the master...
        if (rank == masterRank)
        {
            // Allocate memory to all main matrices
            InitialiseMatrix(m1, size, size, size);
            InitialiseMatrix(m2, size, size, size);
            InitialiseMatrix(m3, size, size, size);

            // Populate input matrices with random values
            PopulateMatrix(m1, size, size);
            PopulateMatrix(m2, size, size);
            
            // Determine the sendcounts and displacement values for each task
            for (int p_id = 0; p_id < numtasks; p_id++)
            {
                displs[p_id] = increment;
                if (size % numtasks != 0 && p_id == 0)
                {
                    // If the size is not divisible by the amount of nodes, the master node will manage the remaining row(s)
                    scatter_rows += (size % numtasks);
                    sendcounts[p_id] = scatter_rows * size;
                }
                else
                    sendcounts[p_id] = scatter_rows * size;
                increment += sendcounts[p_id];
            }

            // Allocate memory for the sub matrices
            InitialiseMatrix(m1_sub, scatter_rows, size, size);
            InitialiseMatrix(m3_sub, scatter_rows, size, size);

            // Scatter data from the m1 matrix into the m1_sub matrices for all nodes using distinct sendcount values
            MPI_Scatterv(&m1[0][0], sendcounts, displs, MPI_INT, &m1_sub[0][0], sendcounts[rank], MPI_INT, masterRank, MPI_COMM_WORLD);
            // Broadcast the entire m2 matrix to all nodes
            MPI_Bcast(&m2[0][0], broadcast_size, MPI_INT, masterRank, MPI_COMM_WORLD);

            // Multiply the m1_sub and m2 matrices and store result in m3_sub (note only need to calculate the count of rows sent to the node)
            MultiplyMatrices(m1_sub, m2, m3_sub, scatter_rows, size);

            // Gather the m3_sub results from all nodes (taking into account their respective sendcount value) and store in m3
            MPI_Gatherv(&m3_sub[0][0], sendcounts[rank], MPI_INT, &m3[0][0], sendcounts, displs, MPI_INT, masterRank, MPI_COMM_WORLD);
        }
        else
        {
            // Determine the sendcounts and displacement values for each task
            for (int p_id = 0; p_id < numtasks; p_id++)
            {
                displs[p_id] = increment;
                if (size % numtasks != 0 && p_id == 0)
                {
                    scatter_rows += (size % numtasks);
                    sendcounts[p_id] = scatter_rows * size;
                }
                else
                    sendcounts[p_id] = scatter_rows * size;
                increment += sendcounts[p_id];
            }
            
            // Allocate memory for the sub matrices and for m2
            InitialiseMatrix(m1_sub, scatter_rows, size, size);
            InitialiseMatrix(m2, size, size, size);
            InitialiseMatrix(m3_sub, scatter_rows, size, size);

            // Receive data from the m1 matrix on master node and store into m1_sub matrix
            MPI_Scatterv(NULL, sendcounts, displs, MPI_INT, &m1_sub[0][0], sendcounts[rank], MPI_INT, masterRank, MPI_COMM_WORLD);
            // Recieve broadcast from the m2 matrix on master
            MPI_Bcast(&m2[0][0], broadcast_size, MPI_INT, masterRank, MPI_COMM_WORLD);

            // Multiply the m1_sub and m2 matrices and store result in m3_sub (note only need to calculate the count of rows sent to the node)
            MultiplyMatrices(m1_sub, m2, m3_sub, scatter_rows, size);
            // Send the m3_sub results to m3 in master
            MPI_Gatherv(&m3_sub[0][0], sendcounts[rank], MPI_INT, NULL, sendcounts, displs, MPI_INT, masterRank, MPI_COMM_WORLD);
        }

        // Retrieve finish time
        auto stop = high_resolution_clock::now();

        // Calculation durations and cast to microseconds
        auto duration = duration_cast<microseconds>(stop - start);

        // Print total time taken on head node
        if (rank == masterRank)
        {
            // Print equation (switched to false - only needed for verify) and time taken
            if (size <= 9)
                PrintEquation(m1, m2, m3, size, true);

            cout << "Time taken to multiply matrices of size " << size << ": " << duration.count() << " microseconds" << endl;
        }
    }	
	// Finalize the MPI environment
	MPI_Finalize();
}