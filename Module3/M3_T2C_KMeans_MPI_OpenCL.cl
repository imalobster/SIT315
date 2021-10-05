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

float CalculateDistance(struct DataPoint p1, struct CentroidPoint p2)
{
	float result = sqrt(pow(p2.x - p1.x, 2) + pow(p2.y - p1.y, 2));
	return result;
}

__kernel void k_means_assignment(const __global int* range, __global struct DataPoint* vectors, __global struct CentroidPoint* centroids)
{
	// Get index and j values from global array
	const int i = get_global_id(0);
	const int j = get_global_id(1); 

    // Set current minimum distance to centroid to maxrange
    vectors[i].curDistance = (float)*range + 1.0f;

    // Get distance between current vector and current centroid
    float newDistance = CalculateDistance(vectors[i], centroids[j]);

    // If new distance is smaller than current distance, assign clusterId
    if (newDistance < vectors[i].curDistance)
    {
        vectors[i].clusterId = centroids[j].clusterId;
        vectors[i].curDistance = newDistance;
    }
}

__kernel void k_means_centroid_update(const __global int* size, const __global struct DataPoint* vectors, __global struct CentroidPoint* centroids, __global int* centroidChanges)
{
	// Get cluster id from global array
	const int j = get_global_id(0);
 
	// Loop through all points in current cluster and sum their x and y
    int xSum = 0;
	int ySum = 0;
	int count = 0;

    for (int i = 0; i < size; i++)
    {
        // If data point belongs to current cluster, add their positions to sum vars
        if (vectors[i].clusterId == centroids[j].clusterId)
        {
            xSum += vectors[i].x;
            ySum += vectors[i].y;
            count++;
        }
    }
    // Only recalculate position if count > 0)
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
        centroidChanges[j] = ((fabs(oldX - centroids[j].x) > e) || (fabs(oldY - centroids[j].y) > e));
    }
}
 