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

void InitialiseMatrix(int** matrix, int rows, int cols, int totalSize)
{
    matrix = (int**)malloc(rows * cols * sizeof(int*));
    int *tempRow = (int*)malloc(rows * cols * sizeof(int));

    for (int i = 0; i < totalSize; i++)
    {
        matrix[i] = &tempRow[i * cols];
    }
}

void PopulateMatrix(int** matrix, int rows, int cols)
{
	for (int i = 0; i < rows; i++)
	{
		for (int j = 0; j < cols; j++)
		{
			matrix[i][j] = rand() % 10;
		}
	}
}

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

void print( int** A, int rows, int cols) {
  for(long i = 0 ; i < rows; i++) { //rows
        for(long j = 0 ; j < cols; j++) {  //cols
            printf("%d ",  A[i][j]); // print the cell value
        }
        printf("\n"); //at the end of the row, print a new line
    }
    printf("----------------------------\n");
}

//Function for creating and initializing a matrix, using a pointer to pointers
void init(int** &matrix, int rows, int cols, int size, bool initialise) 
{
    matrix = (int **) malloc(sizeof(int*) * rows * cols);  
    int* temp = (int *) malloc(sizeof(int) * cols * rows); 

    for(int i = 0 ; i < size ; i++) {
        matrix[i] = &temp[i * cols];
    }
  
    if(!initialise) return;

    for(long i = 0 ; i < rows; i++) {
        for(long j = 0 ; j < cols; j++) {
            matrix[i][j] = rand() % 10; 
        }
    }
}

int **m1;
int **m2;
int **m3;
int size = 3;
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

    // Generate random seed using time
    srand(time(0)); 
    
    // Take current time before populating
    auto startPopulate = high_resolution_clock::now();

    int **m1_sub;

    // Allocate and populate main matrices for head only
    if (rank == masterRank)
    {
        int broadcast_size = size * size;
        int scatter_rows = size / numtasks;
        //scatter_rows += size % numtasks;
        int scatter_size = broadcast_size / numtasks;
        //scatter_size += size % numtasks;

        init(m1, size, size, size, true);
        init(m2, size, size, size, true);
        init(m3, size, size, size, true);
        
        //print(m1, size, size);
        // Populate first two with random variables
        //PopulateMatrix(m1, size, size);
        //PopulateMatrix(m2, size, size);

        int sendcounts[numtasks];
        int displs[numtasks];
        int res = size % numtasks;
        int size_p_process = size / numtasks;
        int increment = 0;

        for (int p_id = 0; p_id < numtasks; p_id++)
        {
            displs[p_id] = increment;
            if (broadcast_size % numtasks != 0 && p_id == 0)
                sendcounts[p_id] = scatter_size + (broadcast_size % numtasks);
            else
                sendcounts[p_id] = scatter_size;
            increment += sendcounts[p_id];
        }

        //printf("rank %d sendcount %d\n", rank, sendcounts[rank]);
        //printf("rank %d displs %d\n", rank, displs[rank]);

        init(m1_sub, scatter_rows, size, size, false);

        print(m1, size, size);
        MPI_Scatterv(&m1[0][0], sendcounts, displs, MPI_INT, &m1_sub[0][0], sendcounts[rank], MPI_INT, masterRank, MPI_COMM_WORLD);
        print(m1_sub, size, size);
        

        printf("rank %d hi\n", rank);
        //MPI_Scatter(&m1[0][0], scatter_size, MPI_INT, &m1, 0, MPI_INT, masterRank, MPI_COMM_WORLD);
        MPI_Bcast(&m2[0][0], broadcast_size, MPI_INT, masterRank, MPI_COMM_WORLD);

        printf("rank %d hi\n", rank);

        MultiplyMatrices(m1_sub, m2, m3, scatter_rows + 1, size);

        printf("rank %d hi\n", rank);

        MPI_Gatherv(MPI_IN_PLACE, sendcounts[rank], MPI_INT, &m3[0][0], sendcounts, displs, MPI_INT, masterRank, MPI_COMM_WORLD);
    }
    else
    {
        int broadcast_size = size * size;
        int scatter_rows = size/numtasks;
        //scatter_rows += size % numtasks;
        int scatter_size = broadcast_size / numtasks;
        //printf("%d, %d, %d\n", broadcast_size, numtasks, scatter_size);

        init(m1, scatter_rows, size, size, true);
        init(m2, size, size, size, false);
        init(m3, scatter_rows, size, size, false);
        

        //PopulateMatrix(m1, scatter_rows, size);

        int sendcounts[numtasks];
        int displs[numtasks];
        int res = size % numtasks;
        int size_p_process = size / numtasks;
        int increment = 0;

        for (int p_id = 0; p_id < numtasks; p_id++)
        {
            displs[p_id] = increment;
            if (broadcast_size % numtasks != 0 && p_id == 0)
                sendcounts[p_id] = scatter_size + (broadcast_size % numtasks);
            else
                sendcounts[p_id] = scatter_size;
            increment += sendcounts[p_id];
        }
        //printf("rank %d sendcount %d\n", rank, sendcounts[rank]);
        //printf("rank %d displs %d\n", rank, displs[rank]);

        MPI_Scatterv(NULL, sendcounts, displs, MPI_INT, &m1[0][0], sendcounts[rank], MPI_INT, masterRank, MPI_COMM_WORLD);

        //print(m1, size, size);

        printf("rank %d hi\n", rank);

        //MPI_Scatter(NULL, scatter_size, MPI_INT, &m1[0][0], scatter_size, MPI_INT, masterRank, MPI_COMM_WORLD);
        MPI_Bcast(&m2[0][0], broadcast_size, MPI_INT, masterRank, MPI_COMM_WORLD);

        printf("rank %d hi\n", rank);

        //print(m2, size, size);

        MultiplyMatrices(m1, m2, m3, scatter_rows, size);

        print(m3, size, size);

        MPI_Gatherv(&m3[0][0], sendcounts[rank], MPI_INT, NULL, sendcounts, displs, MPI_INT, masterRank, MPI_COMM_WORLD);
    }
    

    // Start timer for vector addition process
    auto startMultiply = high_resolution_clock::now();



    // Scatter even portion of data from v1 and v2 in head to sub vectors in all nodes
    
    


    // Multiply first two to produce third matrix


    if (rank == masterRank)
    {
        //print(m3, size, size);
        
    }
    else
    {
        
        //print(m1, size, size);
    }
    

    // Retrieve finish time
    auto stop = high_resolution_clock::now();

    // Calculation durations and cast to microseconds
    auto durationPopulate = duration_cast<microseconds>(startMultiply - startPopulate);
    auto durationMultiply = duration_cast<microseconds>(stop - startMultiply);

    // Print total time taken on head node
    if (rank == masterRank)
    {
        // Print equation (switched to false - only needed for verify) and time taken
        if (size <= 10)
            PrintEquation(m1, m2, m3, size, true);

        cout << "MATRIX SIZE: " << size << endl;
        cout << "Time taken to populate square matrices: " << durationPopulate.count() << " microseconds" << endl;
        cout << "Time taken to multiply square matrices: " << durationMultiply.count() << " microseconds\n" << endl;
    }
    else
    {
        //print(m3, size, size);    
    }
    
	// Finalize the MPI environment
	MPI_Finalize();
}