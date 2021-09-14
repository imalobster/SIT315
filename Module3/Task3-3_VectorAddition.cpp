#include <stdio.h>
#include <stdlib.h>
#include <CL/cl.h>

// Define printing to be on/off (1/0)
#define PRINT 1

// | ------------------------------------------------------ |
// | Variable Declaration									|
// | ------------------------------------------------------ |
// Set size of the vectors
int SZ = 8;
// Declare pointers for the three vectors
int *v1, *v2, *v3;

// Delcare memory buffers for all three vectors
cl_mem bufV1, bufV2, bufV3;

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


// | ------------------------------------------------------ |
// | Function Declaration									|
// | ------------------------------------------------------ |
// Function that returns either the device ID of the GPU (if available) or the CPU
cl_device_id create_device();

// Function that handles the initial setup of the OpenCL environment by setting the device,
// context, program, queue, and kernel variables declared above
void setup_openCL_device_context_queue_kernel(char *filename, char *kernelname);

// Function that creates and builds the OpenGL program from the accompanying .cl file, then
// returns the program handle to the caller
cl_program build_program(cl_context ctx, cl_device_id dev, const char *filename);

// Function that creates a memory buffer to be used by the kernel and writes all vectors data to it
void setup_kernel_memory();

// Function that sets the arguments for the kernel, specifically the vector size and buffer variables
void copy_kernel_args();

// Functions that releases the memory reserved for the above variables back into the available pool
void free_memory();

// Function that initialises a vector with random values
void init(int *&A, int size);

// Function that print
void print(int *A, int size);


// | ------------------------------------------------------ |
// | Main													|
// | ------------------------------------------------------ |
int main(int argc, char **argv)
{
	if (argc > 1)
		SZ = atoi(argv[1]);

	// Populate the initial vectors
	init(v1, SZ);
	init(v2, SZ);

	// Allocate memory for the result vector
	v3 = (int *) malloc (sizeof(int) * SZ);

	// Define variable to store the number of work items per dimension in the work-group
	size_t global[1] = {(size_t)SZ};

	// Print the initial vectors
	print(v1, SZ);
	print(v2, SZ);

	// Setup the OpenGL environment using the handler functions declared above
	setup_openCL_device_context_queue_kernel((char *)"./Task3-3_VectorAddition.cl", (char *)"vector_addition");
	setup_kernel_memory();
	copy_kernel_args();

	// Enqueues the kernel to start executing the commands detailed in the program's queue. It requires the queue and kernel (set up previously),
	// the dimensions of the work-items (in this case 1), offset if applicable, the global work-size array defined above, local work-size if
	// applicable, how many events need to finish before execution can begin (0 in this case), the events to wait on (again, NULL), and the event handler
	clEnqueueNDRangeKernel(queue, kernel, 1, NULL, global, NULL, 0, NULL, &event);

	// Wait for all work-items to finish
	clWaitForEvents(1, &event);

	// Reads memory from buffer objects back to host memory once the program has finished execution. It requires a queue to store the command,
	// the buffer of the vectors, a boolean to block other operations on the memory, offset if necessary, size of the data to be read from the buffer,
	// memory location to read the data into, how many events need to finish before execution can begin (0 in this case), the events to wait on
	// (again, NULL), and the event handler
	clEnqueueReadBuffer(queue, bufV3, CL_TRUE, 0, SZ * sizeof(int), &v3[0], 0, NULL, NULL);

	// Print the resulting vector
	print(v3, SZ);

	// Frees memory of all variables declared above
	free_memory();
}


// | ------------------------------------------------------ |
// | Function Definitions									|
// | ------------------------------------------------------ |
void init(int *&A, int size)
{
	A = (int *)malloc(sizeof(int) * size);

	for (long i = 0; i < size; i++)
	{
		A[i] = rand() % 100; // any number less than 100
	}
}

void print(int *A, int size)
{
	if (PRINT == 0)
	{
		return;
	}

	if (PRINT == 1 && size > 15)
	{
		for (long i = 0; i < 5; i++)
		{                        //rows
			printf("%d ", A[i]); // print the cell value
		}
		printf(" ..... ");
		for (long i = size - 5; i < size; i++)
		{                        //rows
			printf("%d ", A[i]); // print the cell value
		}
	}
	else
	{
		for (long i = 0; i < size; i++)
		{                        //rows
			printf("%d ", A[i]); // print the cell value
		}
	}
	printf("\n----------------------------\n");
}

void free_memory()
{
	// Free the buffers
	clReleaseMemObject(bufV1);
	clReleaseMemObject(bufV2);
	clReleaseMemObject(bufV3);

	// Free OpenCL objects
	clReleaseKernel(kernel);
	clReleaseCommandQueue(queue);
	clReleaseProgram(program);
	clReleaseContext(context);

	// Free the vectors
	free(v1);
	free(v2);
	free(v3);
}


void copy_kernel_args()
{
	// Call the set kernel argument function that stores the location of the data to be used by the kernel. It requires the kernel to
	// store the argument int, the index position of the argument in the kernel, the size of the data, plus a pointer to the data itself
	clSetKernelArg(kernel, 0, sizeof(int), (void *)&SZ);
	clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *)&bufV1);
	clSetKernelArg(kernel, 2, sizeof(cl_mem), (void *)&bufV2);
	clSetKernelArg(kernel, 3, sizeof(cl_mem), (void *)&bufV3);

	if (err < 0)
	{
		perror("Couldn't create a kernel argument");
		printf("error = %d", err);
		exit(1);
	}
}

void setup_kernel_memory()
{
	// The clCreateBuffer function creates in memory a space for a collection of one-dimensional element in a context. It requires the 
	// relevant context used to create the buffer with, a flag denoting the type of buffer, the size of the memory to allocate, as well
	// the location of data already allocated if appicable and an variable to store an error code if necessary.
	// Since we want the kernel to only read from the initials buffer, we specify the CL_MEM_READ_ONLY flag. However for the result buffer, 
	// we only need to write back into it so the CL_MEM_WRITE_ONLY flag is used
	bufV1 = clCreateBuffer(context, CL_MEM_READ_ONLY, SZ * sizeof(int), NULL, NULL);
	bufV2 = clCreateBuffer(context, CL_MEM_READ_ONLY, SZ * sizeof(int), NULL, NULL);
	bufV3 = clCreateBuffer(context, CL_MEM_WRITE_ONLY, SZ * sizeof(int), NULL, NULL);

	// Copy matrices to the devices
	clEnqueueWriteBuffer(queue, bufV1, CL_TRUE, 0, SZ * sizeof(int), &v1[0], 0, NULL, NULL);
	clEnqueueWriteBuffer(queue, bufV2, CL_TRUE, 0, SZ * sizeof(int), &v2[0], 0, NULL, NULL);
	clEnqueueWriteBuffer(queue, bufV3, CL_TRUE, 0, SZ * sizeof(int), &v3[0], 0, NULL, NULL);
}

void setup_openCL_device_context_queue_kernel(char *filename, char *kernelname)
{
	device_id = create_device();
	cl_int err;

	// The clCreateContext function creates a context object that stores configuration information about the OpenGL environment, including
	// the devices, program, and queues. It requires any special properties to be set (NULL in this case), the number of devices, a pointer
	// to a list of unique devices, any callback functions or user data if necessary (both NULL here), and an error code handler
	context = clCreateContext(NULL, 1, &device_id, NULL, NULL, &err);
	if (err < 0)
	{
		perror("Couldn't create a context");
		exit(1);
	}

	program = build_program(context, device_id, filename);

	// The clCreateCommandQueueWithProperties function creates a queue to be used by the kernel to execute commands from on a single device. 
	// It requires a context to be added to, a device ID to be assigned to, any special properties needed for the queue, and an error code handler
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

	// The clCreateProgramWithSource function creates a program in a context and loads all program code into it ready to be used by the
	// queue and kernel. In this case the program code originates from the .cl file. It requires the context to create the program in,
	// the buffer to store the program code in (casted to a char **), the size of the buffer, and an error code handler
	program = clCreateProgramWithSource(ctx, 1,
										(const char **)&program_buffer, &program_size, &err);
	if (err < 0)
	{
		perror("Couldn't create the program");
		exit(1);
	}
	free(program_buffer);

	/* Build program 

   The fourth parameter accepts options that configure the compilation. 
   These are similar to the flags used by gcc. For example, you can 
   define a macro with the option -DMACRO=VALUE and turn off optimization 
   with -cl-opt-disable.
   */
	err = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
	if (err < 0)
	{

		/* Find size of log and print to std output */
		clGetProgramBuildInfo(program, dev, CL_PROGRAM_BUILD_LOG,
							  0, NULL, &log_size);
		program_log = (char *)malloc(log_size + 1);
		program_log[log_size] = '\0';
		clGetProgramBuildInfo(program, dev, CL_PROGRAM_BUILD_LOG,
							  log_size + 1, program_log, NULL);
		printf("%s\n", program_log);
		free(program_log);
		exit(1);
	}

	return program;
}

cl_device_id create_device() {

   cl_platform_id platform;
   cl_device_id dev;
   int err;

   /* Identify a platform */
   err = clGetPlatformIDs(1, &platform, NULL);
   if(err < 0) {
	  perror("Couldn't identify a platform");
	  exit(1);
   } 

   // Access a device
   // GPU
   err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &dev, NULL);
   if(err == CL_DEVICE_NOT_FOUND) {
	  // CPU
	  printf("GPU not found\n");
	  err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_CPU, 1, &dev, NULL);
   }
   if(err < 0) {
	  perror("Couldn't access any devices");
	  exit(1);   
   }

   return dev;
}