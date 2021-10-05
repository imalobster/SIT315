#include <mpi.h>
#include <iostream>
#include <cstdlib>
#include <time.h>
#include <chrono>
#include <CL/cl.h>

using namespace std::chrono;
using namespace std;

// Set rank of master node to 0
#define masterRank 0

// | ------------------------------------------------------ |
// | Variable Declaration									|
// | ------------------------------------------------------ |
// Delcare memory buffers for all three vectors
cl_mem bufM1, bufM2, bufM3;
// Declare variable to store the unique ID of the computational device (GPU, CPU) by a kernel in the program
cl_device_id device_id;
// Declare variable to store the environment configuration (devices, memory properties, queues etc.)
cl_context context;
// Declare variable to store the set of kernels used for the program
cl_program program;
// Declare variable to store the vector addition function which will be executed on a device
cl_kernel kernel;
// Declare variable to store the queue of commands to be executed on a specific device
cl_command_queue queue;
// Declare variable to store event results
cl_event event = NULL;
// Declare variable to keep track of any errors that occur during program execution
int err;

// Create two sub-matrices for internal processing of each node (m2_sub not needed since m2 will be broadcasted)
int *m1_sub;
int *m3_sub;

// Create main matrices for the two inputs and outputs
int *m1;
int *m2;
int *m3;

size_t global[3];

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

void setup_openCL_device_context_queue_kernel(char *filename, char *kernelname)
{
	device_id = create_device();
	cl_int err;

	// Create context object to store configuration details for the OpenCL environment
	context = clCreateContext(NULL, 1, &device_id, NULL, NULL, &err);
	if (err < 0)
	{
		perror("Couldn't create a context");
		exit(1);
	}

	program = build_program(context, device_id, filename);

	// Create queue and kernel for device
	queue = clCreateCommandQueueWithProperties(context, device_id, 0, &err);
	if (err < 0)
	{
		perror("Couldn't create a command queue");
		exit(1);
	};
	kernel = clCreateKernel(program, kernelname, &err);
	if (err < 0)
	{
		perror("Couldn't create a kernel");
		printf("error =%d", err);
		exit(1);
	};
}

void setup_kernel_memory(int rows, int size)
{
	// Create a space in memory for the buffer variables equal to the total size of the sub matrcies
	bufM1 = clCreateBuffer(context, CL_MEM_READ_ONLY, rows * size * sizeof(int), NULL, NULL);
	bufM2 = clCreateBuffer(context, CL_MEM_READ_ONLY, size * size * sizeof(int), NULL, NULL);
	bufM3 = clCreateBuffer(context, CL_MEM_WRITE_ONLY, rows * size * sizeof(int), NULL, NULL);

	// Copy matrices to the devices
	clEnqueueWriteBuffer(queue, bufM1, CL_TRUE, 0, rows * size * sizeof(int), &m1_sub[0], 0, NULL, NULL);
	clEnqueueWriteBuffer(queue, bufM2, CL_TRUE, 0, size * size * sizeof(int), &m2[0], 0, NULL, NULL);
	clEnqueueWriteBuffer(queue, bufM3, CL_TRUE, 0, rows * size * sizeof(int), &m3_sub[0], 0, NULL, NULL);
}

void copy_kernel_args(int size)
{
	// Set kernel arguments needed for the matrix multiplication function
	clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&bufM1);
	clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *)&bufM2);
	clSetKernelArg(kernel, 2, sizeof(cl_mem), (void *)&bufM3);
	clSetKernelArg(kernel, 3, sizeof(int), (void *)&size);

	if (err < 0)
	{
		perror("Couldn't create a kernel argument");
		printf("error = %d", err);
		exit(1);
	}
}

void FreeMemory()
{
	// Free the buffers
	clReleaseMemObject(bufM1);
	clReleaseMemObject(bufM2);
	clReleaseMemObject(bufM3);

	// Free OpenCL objects
	clReleaseKernel(kernel);
	clReleaseCommandQueue(queue);
	clReleaseProgram(program);
	clReleaseContext(context);
}

// This function sets up the OpenCL environment before enqueueing
void SetupOpenCL(int rows, int cols, int size)
{
	// Define variable to store the number of work items per dimension in the work-group
	global[0] = (size_t)rows;
    global[1] = (size_t)cols;
    global[2] = (size_t)size;

	//Setup the OpenGL environment using the handler functions declared above
	setup_openCL_device_context_queue_kernel((char *)"./Task3-T1_MPI_OpenCL.cl", (char *)"matrix_multiply");
	setup_kernel_memory(rows, size);
	copy_kernel_args(size);
}

// This function manages the execution of the OpenCL framework with the cofnigured kernel
void RunOpenCL(int rows, int cols)
{
	// Enqueues the kernel to start executing the commands detailed in the program's queue
	clEnqueueNDRangeKernel(queue, kernel, 3, NULL, global, NULL, 0, NULL, &event);

	// Wait for all work-items to finish
	clWaitForEvents(1, &event);

	// Reads memory from buffer objects back to host memory once the program has finished execution
	clEnqueueReadBuffer(queue, bufM3, CL_TRUE, 0, rows * cols * sizeof(int), &m3_sub[0], 0, NULL, NULL);
}

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
void InitialiseMatrix(int* &matrix, int rows, int cols)
{
	matrix = (int*)malloc(rows * cols * sizeof(int*));

    for (int i = 0; i < rows * cols; i++)
    {
        matrix[i] = 0;
    }
}

// This function populates an input matrix with random integers less than 10
void PopulateMatrix(int* &matrix, int rows, int cols)
{
	for (int i = 0; i < rows * cols; i++)
	{
        matrix[i] = rand() % 10;
	}
}

void print(int* A, int rows, int cols) {
  for(long i = 0 ; i < rows; i++) { //rows
        for(long j = 0 ; j < cols; j++) {  //cols
            int val = (i * cols) + j;
            printf("%d ",  A[val]); // print the cell value

        }
        printf("\n"); //at the end of the row, print a new line
    }
    printf("----------------------------\n");
}

// | ------------------------------------------------------ |
// | Main													|
// | ------------------------------------------------------ |
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

		// Take current time before executing multiplcation
		auto start = high_resolution_clock::now();

		// If running on the master...
		if (rank == masterRank)
		{
			// Allocate memory to all main matrices
			InitialiseMatrix(m1, size, size);
			InitialiseMatrix(m2, size, size);
			InitialiseMatrix(m3, size, size);
        
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
			InitialiseMatrix(m1_sub, scatter_rows, size);
			InitialiseMatrix(m3_sub, scatter_rows, size);
            
            
			// Scatter data from the m1 matrix into the m1_sub matrices for all nodes using distinct sendcount values
			MPI_Scatterv(&m1[0], sendcounts, displs, MPI_INT, &m1_sub[0], sendcounts[rank], MPI_INT, masterRank, MPI_COMM_WORLD);
			// Broadcast the entire m2 matrix to all nodes
			MPI_Bcast(&m2[0], broadcast_size, MPI_INT, masterRank, MPI_COMM_WORLD);

			SetupOpenCL(scatter_rows, size, size);
            RunOpenCL(scatter_rows, size);

            //print(m3_sub, size, size);

            // Gather the m3_sub results from all nodes (taking into account their respective sendcount value) and store in m3
			MPI_Gatherv(&m3_sub[0], sendcounts[rank], MPI_INT, &m3[0], sendcounts, displs, MPI_INT, masterRank, MPI_COMM_WORLD);

			// Free the memory from buffers
			FreeMemory();            
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
			InitialiseMatrix(m1_sub, scatter_rows, size);
			InitialiseMatrix(m2, size, size);
			InitialiseMatrix(m3_sub, scatter_rows, size);

			// Receive data from the m1 matrix on master node and store into m1_sub matrix
			MPI_Scatterv(NULL, sendcounts, displs, MPI_INT, &m1_sub[0], sendcounts[rank], MPI_INT, masterRank, MPI_COMM_WORLD);
			// Recieve broadcast from the m2 matrix on master
			MPI_Bcast(&m2[0], broadcast_size, MPI_INT, masterRank, MPI_COMM_WORLD);

            //print(m1_sub, size, size);

			SetupOpenCL(scatter_rows, size, size);
            RunOpenCL(scatter_rows, size);

			// Send the m3_sub results to m3 in master
			MPI_Gatherv(&m3_sub[0], sendcounts[rank], MPI_INT, NULL, sendcounts, displs, MPI_INT, masterRank, MPI_COMM_WORLD);

			// Free the memory from buffers
			FreeMemory();
		}

		// Retrieve finish time
		auto stop = high_resolution_clock::now();

		// Calculation durations and cast to microseconds
		auto duration = duration_cast<microseconds>(stop - start);

		// Print total time taken on head node
		if (rank == masterRank)
		{
			cout << "Time taken to multiply matrices of size " << size << ": " << duration.count() << " microseconds" << endl;
		}
	}	
	// Finalize the MPI environment
	MPI_Finalize();
}

/* .cl file contents:

// This function computes the multiplication of two matrices and stores the result in a third. It is executed by a kernel that resides with in the
// OpenGL program variable. The instructions are fed via a queue.

__kernel void matrix_multiply(const __global int* matrix1, const __global int* matrix2, __global int* matrix3, const int size)
{
	// Get row, col and size values from global array
	const int i = get_global_id(0);
	const int j = get_global_id(1);
	const int k = get_global_id(2);
 
	// Compute and store the mulitplication result into the output matrix memory
    matrix3[(i * size) + j] += matrix1[(i * size) + k] * matrix2[(k * size) + j];
}

 */