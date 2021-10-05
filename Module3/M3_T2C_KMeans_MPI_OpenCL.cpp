#include <iostream>
#include <cstdlib>
#include <stdio.h>
#include <cmath>
#include <time.h>
#include <chrono>
#include <stdlib.h>
#include <mpi.h>
#include <algorithm>
#include <cmath>
#include <CL/cl.h>

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

// | ------------------------------------------------------ |
// | Variable Declaration									|
// | ------------------------------------------------------ |
// Delcare memory buffers for data point vector and centroids
cl_mem bufV1, bufV1_sub, bufC1, bufCC;
// Declare variable to store the unique ID of the computational device (GPU, CPU) by a kernel in the program
cl_device_id device_id;
// Declare variable to store the environment configuration (devices, memory properties, queues etc.)
cl_context context;
// Declare variable to store the set of kernels used for the program
cl_program program;
// Declare variable to store the centroid assignment and centroid updates functions which will be executed on a device
cl_kernel kernelAssign, kernelUpdate;
// Declare variable to store the queue of commands to be executed on a specific device
cl_command_queue queue;
// Declare variable to store event results
cl_event event = NULL;
// Declare variable to keep track of any errors that occur during program execution
int err;
// Declare global var array for assign and update kernels
size_t globalAssign[2];
size_t globalUpdate[1];

// Declare pointer for data point and centroids vector
DataPoint *vectors;
CentroidPoint *centroids;

// Declare pointer for data point sub vectors
DataPoint *vectors_sub;

// Declare pointer to track centroid changes (implemented as a boolean array)
int *centroidChanges;

// Convergence variable
bool convergence = false;


// | ------------------------------------------------------ |
// | Function Declaration									|
// | ------------------------------------------------------ |
// OpenCL functions are declared here but defined below, otherwise too messy
cl_device_id create_device();
cl_program build_program(cl_context ctx, cl_device_id dev, const char *filename);
void setup_openCL_device_context_queue_kernel(char *filename, char *kernelnameAssign, char *kernelnameUpdate);
void setup_assign_kernel_memory(int size, int k);
void setup_update_kernel_memory(int size, int k);
void copy_assign_kernel_args(int range);
void copy_update_kernel_args(int k);
void CopyAssignKernelData(int size, int k, int range);
void CopyUpdateKernelData(int size, int k);
void RunOpenCLAssign(int size);
bool RunOpenCLUpdate(int k);
void SetupOpenCL(int size, int k);
void PostExecutionCleanup();


// | ------------------------------------------------------ |
// | Program Function Definition							|
// | ------------------------------------------------------ |
// I decided to define program functions here to differentiate between OpenCL functions

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

// | ------------------------------------------------------ |
// | Main													|
// | ------------------------------------------------------ |
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

		// Get the current time before clustering algorithm begins
		auto start = high_resolution_clock::now();


		if (rank == masterRank)
		{
			// Allocate memory to vectors (I ran into issues using malloc for size greater than 924 so resorted to 'new')
			vectors = new DataPoint[size];
			centroids = new CentroidPoint[k];

            // Allocate memory for the pseudo-boolean centroid change array
            centroidChanges = new int[k];

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

        // Setup the OpenCL program, devices, queues etc
        SetupOpenCL(scatter_vals, k);

        while (!convergence)
        {
            // Broadcast array of centroids to all nodes
			MPI_Bcast(&centroids[0], k * sizeof(CentroidPoint), MPI_BYTE, masterRank, MPI_COMM_WORLD);
            MPI_Scatterv(&vectors[0], sendcounts, displs, MPI_BYTE, &vectors_sub[0], sendcounts[rank], MPI_BYTE, masterRank, MPI_COMM_WORLD);

            // Copy across updated data to buffers
            CopyAssignKernelData(scatter_vals, k, range);

            // Run the kernel, wait for all to finish, then copy buffers to original memory locations
            RunOpenCLAssign(scatter_vals);

            // Gather all results back to master node for centroid calculation
            if (rank == masterRank)
            {
                MPI_Gatherv(&vectors_sub[0], sendcounts[rank], MPI_BYTE, &vectors[0], sendcounts, displs, MPI_BYTE, masterRank, MPI_COMM_WORLD);
            }
            else
            {
                MPI_Gatherv(&vectors_sub[0], sendcounts[rank], MPI_BYTE, NULL, sendcounts, displs, MPI_BYTE, masterRank, MPI_COMM_WORLD);
            }
			// Recalculate cluster and check for convergence
			if (rank == masterRank)
			{
                CopyUpdateKernelData(size, k);
                convergence = RunOpenCLUpdate(k);
			}
            
			// Broadcast convergence result back to worker nodes (to stop their loops)
			MPI_Bcast(&convergence, 1, MPI_CXX_BOOL, masterRank, MPI_COMM_WORLD);  
        }

        if (rank == masterRank)
        {
            //Print(vectors, size);
            //Print(centroids, k);
        }

        // Release buffer and array memory
        PostExecutionCleanup();
		
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

// | ------------------------------------------------------ |
// | OpenCL Function Definition								|
// | ------------------------------------------------------ |
cl_device_id create_device()
{
	cl_platform_id platform;
	cl_device_id dev;
	int err;

	/* Identify a platform */
	err = clGetPlatformIDs(1, &platform, NULL);
	if(err < 0) 
	{
		perror("Couldn't identify a platform");
		exit(1);
	} 
	// Access a device
	// GPU
	err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &dev, NULL);
	if(err == CL_DEVICE_NOT_FOUND) 
	{
		// CPU
		//printf("GPU not found\n");
		err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_CPU, 1, &dev, NULL);
	}
	if(err < 0) 
	{
		perror("Couldn't access any devices");
		exit(1);   
	}
	return dev;
}

cl_program build_program(cl_context ctx, cl_device_id dev, const char *filename)
{

	cl_program program;
	FILE *program_handle;
	char *program_buffer, *program_log;
	size_t program_size, log_size;

	/* Read program file and place content into buffer */
	program_handle = fopen(filename, "r");
	if (program_handle == NULL)
	{
		perror("Couldn't find the program file");
		exit(1);
	}
	fseek(program_handle, 0, SEEK_END);
	program_size = ftell(program_handle);
	rewind(program_handle);
	program_buffer = (char *)malloc(program_size + 1);
	program_buffer[program_size] = '\0';
	fread(program_buffer, sizeof(char), program_size, program_handle);
	fclose(program_handle);

	program = clCreateProgramWithSource(ctx, 1, (const char **)&program_buffer, &program_size, &err);
	if (err < 0)
	{
		perror("Couldn't create the program");
		exit(1);
	}
	free(program_buffer);

	err = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
	if (err < 0)
	{

		/* Find size of log and print to std output */
		clGetProgramBuildInfo(program, dev, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
		program_log = (char *)malloc(log_size + 1);
		program_log[log_size] = '\0';
		clGetProgramBuildInfo(program, dev, CL_PROGRAM_BUILD_LOG, log_size + 1, program_log, NULL);
		printf("%s\n", program_log);
		free(program_log);
		exit(1);
	}
	return program;
}

void setup_openCL_device_context_queue_kernel(char *filename, char *kernelnameAssign, char *kernelnameUpdate)
{
    device_id = create_device();
    cl_int err;

    //ToDo: Add comment (what is the purpose of clCreateBuffer function?)
    context = clCreateContext(NULL, 1, &device_id, NULL, NULL, &err);
    if (err < 0)
    {
        perror("Couldn't create a context");
        exit(1);
    }

    program = build_program(context, device_id, filename);

    //ToDo: Add comment (what is the purpose of clCreateCommandQueueWithProperties function?)
    queue = clCreateCommandQueueWithProperties(context, device_id, 0, &err);
    if (err < 0)
    {
        perror("Couldn't create a command queue");
        exit(1);
    };

    kernelAssign = clCreateKernel(program, kernelnameAssign, &err);
    if (err < 0)
    {
        perror("Couldn't create a kernel");
        printf("error =%d", err);
        exit(1);
    };

    kernelUpdate = clCreateKernel(program, kernelnameUpdate, &err);
    if (err < 0)
    {
        perror("Couldn't create a kernel");
        printf("error =%d", err);
        exit(1);
    };
}

void setup_assign_kernel_memory(int size, int k)
{
	// Create a space in memory for the buffer variables equal to the total size of the sub data point vector and centroid vector
	bufV1_sub = clCreateBuffer(context, CL_MEM_READ_WRITE, size * sizeof(DataPoint), NULL, NULL);
	bufC1 = clCreateBuffer(context, CL_MEM_READ_ONLY, k * sizeof(CentroidPoint), NULL, NULL);
	
	// Copy vectors to the devices
	clEnqueueWriteBuffer(queue, bufV1_sub, CL_TRUE, 0, size * sizeof(DataPoint), &vectors_sub[0], 0, NULL, NULL);
	clEnqueueWriteBuffer(queue, bufC1, CL_TRUE, 0, k * sizeof(CentroidPoint), &centroids[0], 0, NULL, NULL);
}

void setup_update_kernel_memory(int size, int k)
{
	// Create a space in memory for the buffer variables equal to the total size of the sub data point vector and centroid vector
	bufV1 = clCreateBuffer(context, CL_MEM_READ_ONLY, size * sizeof(DataPoint), NULL, NULL);
	bufC1 = clCreateBuffer(context, CL_MEM_READ_WRITE, k * sizeof(CentroidPoint), NULL, NULL);
    bufCC = clCreateBuffer(context, CL_MEM_WRITE_ONLY, k * sizeof(int), NULL, NULL);
	
	// Copy vectors to the devices
	clEnqueueWriteBuffer(queue, bufV1, CL_TRUE, 0, size * sizeof(DataPoint), &vectors[0], 0, NULL, NULL);
	clEnqueueWriteBuffer(queue, bufC1, CL_TRUE, 0, k * sizeof(CentroidPoint), &centroids[0], 0, NULL, NULL);
    clEnqueueWriteBuffer(queue, bufCC, CL_TRUE, 0, k * sizeof(int), &centroidChanges[0], 0, NULL, NULL);
}

void copy_assign_kernel_args(int range)
{
    // Pass the addresses of the structures needs for the vector assignment kernel
    clSetKernelArg(kernelAssign, 0, sizeof(int), (void *)&range);
    clSetKernelArg(kernelAssign, 1, sizeof(cl_mem), (void *)&bufV1_sub);
    clSetKernelArg(kernelAssign, 2, sizeof(cl_mem), (void *)&bufC1);

    if (err < 0)
    {
        perror("Couldn't create a kernel argument");
        printf("error = %d", err);
        exit(1);
    }
}

void copy_update_kernel_args(int size)
{
    // Pass the addresses of the structures needs for the centroid update kernel
    clSetKernelArg(kernelUpdate, 0, sizeof(int), (void *)&size);
    clSetKernelArg(kernelUpdate, 1, sizeof(cl_mem), (void *)&bufV1);
    clSetKernelArg(kernelUpdate, 2, sizeof(cl_mem), (void *)&bufC1);
    clSetKernelArg(kernelUpdate, 3, sizeof(cl_mem), (void *)&bufCC);

    if (err < 0)
    {
        perror("Couldn't create a kernel argument");
        printf("error = %d", err);
    }
        
}

void PostExecutionCleanup()
{
	// Free the buffers
    clReleaseMemObject(bufV1);
    clReleaseMemObject(bufV1_sub);
	clReleaseMemObject(bufC1);
    clReleaseMemObject(bufCC);

	// Free OpenCL objects
	clReleaseKernel(kernelAssign);
    clReleaseKernel(kernelUpdate);
	clReleaseCommandQueue(queue);
	clReleaseProgram(program);
	clReleaseContext(context);

    // Delete array data
    delete[] vectors;
    delete[] vectors_sub;
    delete[] centroids;
    delete[] centroidChanges;

    // Set convergence back to false for next loop
    convergence = false;
}

// This function sets up the OpenCL environment before enqueueing
void SetupOpenCL(int size, int k)
{
	// Store work item counts in global assign array   
    globalAssign[0] = (size_t)size;
    globalAssign[1] = (size_t)k;

    // Store work item counts for the global update array
    globalUpdate[0] = (size_t)k;

	//Setup the OpenGL environment using the handler functions declared above
    setup_openCL_device_context_queue_kernel((char *)"./M3_T2C_KMeans_MPI_OpenCL.cl", (char *)"k_means_assignment", (char *)"k_means_centroid_update");
}

void CopyAssignKernelData(int size, int k, int range)
{
    setup_assign_kernel_memory(size, k);
    copy_assign_kernel_args(range);
}

void CopyUpdateKernelData(int size, int k)
{
    // Fill changes array with false (0) every iteration
    fill_n(centroidChanges, k, 0);

    setup_update_kernel_memory(size, k);
    copy_update_kernel_args(size);
}

// This function manages the execution of the OpenCL framework with the cofnigured kernel
void RunOpenCLAssign(int size)
{
	// Enqueues the kernel to start executing the commands detailed in the program's queue
    clEnqueueNDRangeKernel(queue, kernelAssign, 2, NULL, globalAssign, NULL, 0, NULL, &event);

	// Wait for all work-items to finish
	clWaitForEvents(2, &event);

	//Reads memory from buffer objects back to host memory once the program has finished execution
	clEnqueueReadBuffer(queue, bufV1_sub, CL_TRUE, 0, size * sizeof(DataPoint), &vectors_sub[0], 0, NULL, NULL);
}

// This function manages the execution of the OpenCL framework with the cofnigured kernel
bool RunOpenCLUpdate(int k)
{
	// Enqueues the kernel to start executing the commands detailed in the program's queue
    clEnqueueNDRangeKernel(queue, kernelUpdate, 1, NULL, globalUpdate, NULL, 0, NULL, &event);

	// Wait for all work-items to finish
	clWaitForEvents(1, &event);

	//Reads memory from buffer objects back to host memory once the program has finished execution
    clEnqueueReadBuffer(queue, bufC1, CL_TRUE, 0, k * sizeof(CentroidPoint), &centroids[0], 0, NULL, NULL);
	clEnqueueReadBuffer(queue, bufCC, CL_TRUE, 0, k * sizeof(int), &centroidChanges[0], 0, NULL, NULL);

    for (int i = 0; i < k; i++)
    {
        if (centroidChanges[i] == 1)
        {
            //printf("Centroid change detected\n");
            return false;
        }
    }
    return true;
}
