// This function computes the addition of two vectors and stores the result in a third. It is executed by a kernel that resides with in the
// OpenGL program variable. The instructions are fed via a queue.

__kernel void vector_addition(const int size, const __global int* v1,const __global int* v2,__global int* v3)
{
	// Thread identifiers
	const int globalIndex = get_global_id(0);    
 
	//uncomment to see the index each PE works on
	//printf("Kernel process index :(%d)\n ", globalIndex);

	v3[globalIndex] = v2[globalIndex] + v1[globalIndex];
}
 