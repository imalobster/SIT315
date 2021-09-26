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
 