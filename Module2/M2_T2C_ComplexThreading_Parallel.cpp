#include <iostream>
#include <cstdlib>
#include <cmath>
#include <time.h>
#include <chrono>
#include <stdlib.h>
#include <windows.h>
#include <omp.h>

using namespace std::chrono;
using namespace std;

// Define number of threads
const int NUM_THREADS = 8;

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
	#pragma omp parallel shared(vectors) firstprivate(size, maxRange)
	{
		#pragma omp for schedule(auto)
		for (int i = 0; i < size; i++)
		{
			// Generate random coordinates between 0 and max_range for each data point
			vectors[i].x = rand() % maxRange;
			vectors[i].y = rand() % maxRange;
			vectors[i].curDistance = maxRange + 1.0;
		}
	}
}

// Initialises centroids with random values in preparation for clustering algorithm
void InitialiseCentroidPoints(CentroidPoint *centroids, int size, int maxRange)
{
	#pragma omp parallel shared(centroids) firstprivate(size, maxRange)
	{
		#pragma omp for schedule(auto)
		for (int i = 0; i < size; i++)
		{
			// Generate random coordinates between 0 and max_range for each data point
			centroids[i].x = rand() % maxRange;
			centroids[i].y = rand() % maxRange;
			centroids[i].clusterId = i;
		}
	}
}

// Calculates the euclidian distance between a data point and a centroid point
float CalculateDistance(DataPoint p1, CentroidPoint p2)
{
	float result = sqrt(pow(p2.x - p1.x, 2) + pow(p2.y - p1.y, 2));
	return result;
}

// Assign data points to a new centroid based on their euclidian distance
bool AssignCentroids(DataPoint *vectors, CentroidPoint *centroids, int vSize, int cSize)
{
	// Declare boolean to determine if any points changed cluster
	bool changed = false;

	// Calaculate distance and assign data points to centroid
	#pragma omp parallel shared(vectors, centroids) firstprivate(vSize, cSize)
	{
		#pragma omp for collapse(2) schedule(auto)
		for (int i = 0; i < vSize; i++)
		{
			// Loop through all centroids
			for (int j = 0; j < cSize; j++)
			{
				// Get distance between current vector and current centroid
				float newDistance = CalculateDistance(vectors[i], centroids[j]);

				// If new distance is smaller than current distance, assign clusterId
				if (newDistance < vectors[i].curDistance)
				{
					int oldClusterId = vectors[i].clusterId;
					vectors[i].clusterId = centroids[j].clusterId;

					// If cluster ids changed, set change to true
					if (oldClusterId != vectors[i].clusterId)
					{
						changed = true;
					}
				}
			}
		}
	}
	return !changed;
}

// Calculates new mean of centroids after new points assigned (returns true if no changes)
void CalculateNewCentroids(DataPoint *vectors, CentroidPoint *centroids, int vSize, int cSize)
{
	unsigned long totalReduction = 0;
	#pragma omp parallel shared(vectors, centroids) firstprivate(vSize, cSize)
	{
		#pragma omp for schedule(auto)
		for (int j = 0; j < cSize; j++)
		{
			// Loop through all points in current cluster and sum their x and y
			int xSum = 0;
			int ySum = 0;
			int count = 0;

			for (int i = 0; i < vSize; i++)
			{
				if (vectors[i].clusterId == centroids[j].clusterId)
				{
					xSum += vectors[i].x;
					ySum += vectors[i].y;
					count++;
				}
			}
			// Calculate new x and y values based on mean sum
			centroids[j].x = (float)xSum / count;
			centroids[j].y = (float)ySum / count;
		}

		// Update each data points distance to assigned centroid
		#pragma omp for schedule(auto)
		for (int i = 0; i < vSize; i++)
		{
			vectors[i].curDistance = CalculateDistance(vectors[i], centroids[vectors[i].clusterId]);
		}
	}
}

int main(){
	// Define range of sizes to test (i.e how many data points)
	int n_sizes[] = { 1000, 10000, 100000, 1000000 };

	for (int size : n_sizes)
	{
		// Define count of k-means centroids
		int k = 3;
		// Define max range of coordinates
		int range = 1000;

		// Set random seed based on current time
		srand(time(0));

		// Define pointer for data point and centroids vector (as well as centroidsBefore to compare the change)
		DataPoint *vectors;
		CentroidPoint *centroids;

		// Get the current time before clustering algorithm begins
		auto start = high_resolution_clock::now();

		// Allocate memory to vectors (I ran into issues using malloc for size greater than 924 so resorted to 'new')
		vectors = new DataPoint[size];
		centroids = new CentroidPoint[k];

		// Set number of threads for OpenMP
		omp_set_num_threads(NUM_THREADS);

		// Initialise random data points and centroids
		InitialiseDataPoints(vectors, size, range);
		InitialiseCentroidPoints(centroids, k, range);

		// Run clustering algorithm until convergence is reach
		bool convergence = false;

		while (!convergence)
		{
			// Assign data points to centroids (if nothing changes, convergence will be set to true)
			convergence = AssignCentroids(vectors, centroids, size, k);

			// Recalculate cluster centroids provided we haven't converged
			if (!convergence)
			{
				CalculateNewCentroids(vectors, centroids, size, k);
			}
		}
		
		// Get the current time after vector assignment
		auto stop = high_resolution_clock::now();

		// Obtain the difference between start and stop times, then cast to microseconds format
		auto duration = duration_cast<microseconds>(stop - start);

		cout << "Size " << size << " execution time: "
			<< duration.count() << " microseconds" << endl;


		bool print = false;
		if (print)
		{
			for (int i = 0; i < size; i++)
			{
				printf("%d, %d, %d \n", vectors[i].x, vectors[i].y, vectors[i].clusterId);
			}
			printf("\n");
			for (int i = 0; i < k; i++)
			{
				printf("%f, %f, %d \n", centroids[i].x, centroids[i].y, centroids[i].clusterId);
			}
		}
	}
	return 0;
}