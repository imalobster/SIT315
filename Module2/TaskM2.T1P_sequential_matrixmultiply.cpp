#include <iostream>
#include <fstream>
#include <cstdlib>
#include <time.h>
#include <chrono>
#include <stdio.h>

using namespace std::chrono;
using namespace std;

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

int main()
{
	// Delete any existing results file
	remove("results_sequential.txt");

	// Define sizes of matrices
	int sizes[] = { 10, 100, 1000 };

	for (int size : sizes)
	{
		// Define matrix size
		int matrixSize = size;

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

		// Take current time before populating
		auto startPopulate = high_resolution_clock::now();

		// Populate first two with random variables
		PopulateMatrix(m1, matrixSize);
		PopulateMatrix(m2, matrixSize);
		
		// Take current time before multiplication
		auto startMultiply = high_resolution_clock::now();

		// Multiply first two to produce third matrix
		MultiplyMatrices(m1, m2, m3, matrixSize);

		// Take current time before multiplication
		auto stop = high_resolution_clock::now();

		// Calculation durations and cast to microseconds
		auto durationPopulate = duration_cast<microseconds>(startMultiply - startPopulate);
		auto durationMultiply = duration_cast<microseconds>(stop - startMultiply);

		// Print equation (switched to false - only needed for verify) and time taken
		if (matrixSize <= 10)
			PrintEquation(m1, m2, m3, matrixSize, true);
		cout << "MATRIX SIZE: " << size << endl;
		cout << "Time taken to populate square matrices: " << durationPopulate.count() << " microseconds" << endl;
		cout << "Time taken to multiply square matrices: " << durationMultiply.count() << " microseconds\n" << endl;

		// Redirect stdout to file and call above again
		freopen("results_sequential.txt", "a", stdout);
		cout << "MATRIX SIZE: " << size << endl;
		cout << "Time taken to populate square matrices: " << durationPopulate.count() << " microseconds" << endl;
		cout << "Time taken to multiply square matrices: " << durationMultiply.count() << " microseconds\n" << endl;

		// Redirect stdout to terminal again
		freopen("CON", "w", stdout);
	}

	return 0;
}