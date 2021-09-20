#include <iostream>
#include <cstdlib>
#include <cmath>
#include <time.h>
#include <chrono>
#include <stdlib.h>
#include <windows.h>

using namespace std::chrono;
using namespace std;

// Define struct to hold coordinates of each data point
struct DataPoint
{
	float x;
	float y;
	float curDistance;
	int clusterId;
};

// Initialises vectors with random values in preparation for clustering algorithm
void InitialiseDataPoints(DataPoint *vectors, int size, int maxRange, bool centroid)
{
	for (int i = 0; i < size; i++)
	{
		// Generate random coordinates between 0 and max_range for each data point
		vectors[i].x = rand() % maxRange;
		vectors[i].y = rand() % maxRange;
		vectors[i].curDistance = maxRange + 1.0;
		
		// If initialising centroid, assign id to each
		if (centroid)
		{
			vectors[i].clusterId = i;
		}
	}
}

// Calculates the euclidian distance between two points
float CalculateDistance(DataPoint p1, DataPoint p2)
{
	float result = sqrt(pow(p2.x - p1.x, 2) + pow(p2.y - p1.y, 2));
	return result;
}

// Assign data points to a new centroid based on their euclidian distance
bool AssignCentroids(DataPoint *vectors, DataPoint *centroids, int vSize, int cSize, int maxRange)
{
	// Declare boolean to determine if any points changed cluster
	bool changed = false;

	// Calaculate distance and assign data points to centroid
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
				vectors[i].curDistance = newDistance;
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
	return !changed;
}

// Calculates new mean of centroids after new points assigned (returns true if no changes)
void CalculateNewCentroids(DataPoint *vectors, DataPoint *centroids, int vSize, int cSize)
{
	for (int j = 0; j < cSize; j++)
	{
		// Loop through all points in current cluster and sum their x and y
		float xSum = 0.0;
		float ySum = 0.0;
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
		centroids[j].x = xSum / count;
		centroids[j].y = ySum / count;
	}
}

int main(){
	// Define range of sizes to test (i.e how many data points)
	int n_sizes[] = { 10000, 100000, 1000000 };

	for (int size : n_sizes)
	{
		// Define count of k-means centroids
		int k = 3;
		// Define max range of coordinates
		int range = 1000000;

		// Set random seed based on current time
		srand(time(0));

		// Define pointer for data point and centroids vector (as well as centroidsBefore to compare the change)
		DataPoint *vectors;
		DataPoint *centroids;
		DataPoint *centroidsBefore;

		// Get the current time before clustering algorithm begins
		auto start = high_resolution_clock::now();

		// Allocate memory to vectors (I ran into issues using malloc for size greater than 924 so resorted to 'new')
		vectors = new DataPoint[size];
		centroids = new DataPoint[k];
		centroidsBefore = new DataPoint[k];

		// Initialise random data points and centroids
		InitialiseDataPoints(vectors, size, range, false);
		InitialiseDataPoints(centroids, k, range, true);

		// Store original values of centroids
		centroidsBefore = centroids;

		// Run clustering algorithm until convergence is reach
		bool convergence = false;

		while (!convergence)
		{
			// Assign data points to centroids (if nothing changes, convergence will be set to true)
			convergence = AssignCentroids(vectors, centroids, size, k, range);

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
	}
	return 0;
}