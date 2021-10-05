#include <iostream>
#include <cstdlib>
#include <cmath>
#include <time.h>
#include <chrono>
#include <stdlib.h>
#include <mpi.h>
#include <algorithm>
#include <cmath>

using namespace std::chrono;
using namespace std;

// Set rank of master node to 0
#define masterRank 0

// Define struct to hold coordinates of each data point
struct DataPoint
{
	int x;
	int y;
	float curDistance;
	int clusterId;
};

// Define struct to hold coordinates of each centroid point
struct CentroidPoint
{
	float x;
	float y;
	int clusterId;
};

// Initialises vectors with random values in preparation for clustering algorithm
void InitialiseDataPoints(DataPoint *vectors, int size, int maxRange)
{
	for (int i = 0; i < size; i++)
	{
		// Generate random coordinates between 0 and max_range for each data point
		vectors[i].x = rand() % maxRange;
		vectors[i].y = rand() % maxRange;
		vectors[i].curDistance = maxRange + 1.0;
	}
}

// Initialises centroids with random values in preparation for clustering algorithm
void InitialiseCentroidPoints(CentroidPoint *centroids, int size, int maxRange)
{
	for (int i = 0; i < size; i++)
	{
		// Generate random coordinates between 0 and max_range for each data point
		centroids[i].x = rand() % maxRange;
		centroids[i].y = rand() % maxRange;
		centroids[i].clusterId = i;
	}
}

// Calculates the euclidian distance between a data point and a centroid point
float CalculateDistance(DataPoint p1, CentroidPoint p2)
{
	float result = sqrt(pow(p2.x - p1.x, 2) + pow(p2.y - p1.y, 2));
	return result;
}

// Assign data points to a new centroid based on their euclidian distance
void AssignCentroids(DataPoint *vectors, CentroidPoint *centroids, int vSize, int cSize, int maxRange)
{
	// Calaculate distance and assign data points to centroid
	for (int i = 0; i < vSize; i++)
	{
		// Reset distance to be max range to account for centroid position changes
		vectors[i].curDistance = maxRange + 1;

		// Loop through all centroids
		for (int j = 0; j < cSize; j++)
		{
			// Get distance between current vector and current centroid
			float newDistance = CalculateDistance(vectors[i], centroids[j]);


			// If new distance is smaller than current distance, assign clusterId
			if (newDistance < vectors[i].curDistance)
			{
				vectors[i].clusterId = centroids[j].clusterId;
				vectors[i].curDistance = newDistance;
			}
		}
	}
}

// Calculates new mean of centroids after new points assigned (returns true if no changes)
bool CalculateNewCentroids(DataPoint *vectors, CentroidPoint *centroids, int vSize, int cSize)
{
	// Create array to store change results in
	bool centroidChange[cSize];
	fill_n(centroidChange, cSize, false);

	// Loop through each centroid
	for (int j = 0; j < cSize; j++)
	{
		// Loop through all points in current cluster and sum their x and y
		int xSum = 0;
		int ySum = 0;
		int count = 0;

		for (int i = 0; i < vSize; i++)
		{
			// If data point belongs to current cluster, add their positions to sum vars
			if (vectors[i].clusterId == centroids[j].clusterId)
			{
				xSum += vectors[i].x;
				ySum += vectors[i].y;
				count++;
			}
		}
		if (count > 0)
		{
			// Get existing x and y values
			float oldX = centroids[j].x;
			float oldY = centroids[j].y;

			// Calculate new x and y values based on mean sum
			centroids[j].x = (float)xSum / count;
			centroids[j].y = (float)ySum / count;

			// Check if they changed by epsilon value and if so record that change
			float e = 0.005f;
			centroidChange[j] = ((fabs(oldX - centroids[j].x) > e) || (fabs(oldY - centroids[j].y) > e));
		}
	}

	// If change detected then return false (since convergence will thus equal false)
	for (int j = 0; j < cSize; j++)
	{
		if (centroidChange[j])
			return false;
	}

	// Otherwise return true (no change detected)
	return true;
}

void Print(DataPoint* vectors, int size)
{
	for (int i = 0; i < size; i++)
	{
		printf("%d, %d, %d, %f \n", vectors[i].x, vectors[i].y, vectors[i].clusterId, vectors[i].curDistance);
	}
	printf("\n");
}

void Print(CentroidPoint* centroids, int size)
{
	for (int i = 0; i < size; i++)
	{
		printf("%f, %f, %d \n", centroids[i].x, centroids[i].y, centroids[i].clusterId);
	}
	printf("\n");
}

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
		
	// Define range of sizes to test (i.e how many data points)
	int n_sizes[] = { 1, 1, 10, 10, 100, 1000, 10000, 100000, 1000000 };

	for (int size : n_sizes)
	{
		// Define count of k-means centroids
		int k = 3;
		// Define max range of coordinates
		int range = 1000;

		// Calculate how many rows to send to each node
		int scatter_vals = size / numtasks;
		// Create an array to store how much data to send across to each node
		int sendcounts[numtasks];
		// Create an array to store the displacement values used for keeping track of data sent for each node
		int displs[numtasks];
		// Increment variable to store the most recent displacement value
		int increment = 0;

		// Set random seed based on current time
		srand(time(0));

		// Declare pointer for data point and centroids vector
		DataPoint *vectors;
		CentroidPoint *centroids;

		// Declare pointer for data point sub vectors
		DataPoint *vectors_sub;

		// Get the current time before clustering algorithm begins
		auto start = high_resolution_clock::now();

		if (rank == masterRank)
		{
			// Allocate memory to vectors (I ran into issues using malloc for size greater than 924 so resorted to 'new')
			vectors = new DataPoint[size];
			centroids = new CentroidPoint[k];

			// Initialise random data points and centroids
			InitialiseDataPoints(vectors, size, range);
			InitialiseCentroidPoints(centroids, k, range);

			// Determine the sendcounts and displacement values for each task
			for (int p_id = 0; p_id < numtasks; p_id++)
			{
				displs[p_id] = increment;
				if (size % numtasks != 0 && p_id == masterRank)
				{
					// If the size is not divisible by the amount of nodes, the master node will manage the remaining row(s)
					sendcounts[p_id] = (scatter_vals + (size % numtasks)) * sizeof(DataPoint);
				}
				else
				{
					sendcounts[p_id] = scatter_vals * sizeof(DataPoint);
				}
				increment += sendcounts[p_id];
			}

			// Since we are the master node, we will need to update the scatter rows to reflect additional work
			scatter_vals += (size % numtasks);

			// Allocate memory for the sub data point vector
			vectors_sub = new DataPoint[scatter_vals];
		}
		else
		{
			// Determine the sendcounts and displacement values for each task
			for (int p_id = 0; p_id < numtasks; p_id++)
			{
				displs[p_id] = increment;
				if (size % numtasks != 0 && p_id == masterRank)
				{
					// If the size is not divisible by the amount of nodes, the master node will manage the remaining row(s)
					sendcounts[p_id] = (scatter_vals + (size % numtasks)) * sizeof(DataPoint);
				}
				else
				{
					sendcounts[p_id] = scatter_vals * sizeof(DataPoint);
				}
				increment += sendcounts[p_id];
			}

			// Allocate memory for the sub data point vector
			vectors_sub = new DataPoint[scatter_vals];

			// Allocate memory for centroids to be broadcasted into
			centroids = new CentroidPoint[k];
		}

		// Run clustering algorithm until convergence is reached
		int convergence = false;

		while (!convergence)
		{
			// Broadcast array of centroids to all nodes
			MPI_Bcast(&centroids[0], k * sizeof(CentroidPoint), MPI_BYTE, masterRank, MPI_COMM_WORLD);

			// Distribute data points vector to worker nodes
			MPI_Scatterv(&vectors[0], sendcounts, displs, MPI_BYTE, &vectors_sub[0], sendcounts[rank], MPI_BYTE, masterRank, MPI_COMM_WORLD);

			// Assign data points to centroids
			AssignCentroids(vectors_sub, centroids, scatter_vals, k, range);
		   
			if (rank == masterRank)
			{
				// Send results of all sub vectors back to original vectors array
				MPI_Gatherv(&vectors_sub[0], sendcounts[rank], MPI_BYTE, &vectors[0], sendcounts, displs, MPI_BYTE, masterRank, MPI_COMM_WORLD);
			}
			else
			{
				// Send back results of sub vector to master
				MPI_Gatherv(&vectors_sub[0], sendcounts[rank], MPI_BYTE, NULL, sendcounts, displs, MPI_BYTE, masterRank, MPI_COMM_WORLD);
			}

			// Recalculate cluster centroids provided we haven't converged
			if (rank == masterRank)
			{
				convergence = CalculateNewCentroids(vectors, centroids, size, k);
			}

			// Broadcast convergence result back to worker nodes (to stop their loops)
			MPI_Bcast(&convergence, 1, MPI_CXX_BOOL, masterRank, MPI_COMM_WORLD);          
		}
		
		// Get the current time after vector assignment
		auto stop = high_resolution_clock::now();

		// Obtain the difference between start and stop times, then cast to microseconds format
		auto duration = duration_cast<microseconds>(stop - start);

		if (rank == masterRank)
		{
			cout << "Size " << size << " execution time: "
				<< duration.count() << " microseconds" << endl;
		}
	}
	// Finalize the MPI environment
	MPI_Finalize();
}