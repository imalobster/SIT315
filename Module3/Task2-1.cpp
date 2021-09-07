#include <mpi.h>
#include <stdio.h>
#include <cstring>
#include <stdlib.h>
#include <string>
using namespace std;

#define masterRank 0

int main(int argc, char** argv) {
	// Initalise MPI variables 
	int numtasks, rank, name_len, tag=1;
	// Define char array for processor name
	char name[MPI_MAX_PROCESSOR_NAME];
	// Initialize the MPI environment
	MPI_Init(&argc,&argv);
	// Get the number of tasks/process
	MPI_Comm_size(MPI_COMM_WORLD, &numtasks);
	// Get the rank
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	// Find the processor name
	MPI_Get_processor_name(name, &name_len);

	// Define var for message to send
	string newMsg;
	// Define var for size of message
	int msgSize;
	if (rank == masterRank)
	{
		// Define message text and size in head
		newMsg = "hello world";
		msgSize = newMsg.size();
		
		// Send message to all ranks from head (note c_str() returns pointer to array of char from original string)
		for (int i = 1; i < numtasks; i++)
		{
			MPI_Send(newMsg.c_str(), msgSize, MPI_CHAR, i, 0, MPI_COMM_WORLD);
		}
	}
	else
	{
		// Dynamically determine size of char array by using the probe and get_count methods
		MPI_Status status;
		MPI_Probe(0, 0, MPI_COMM_WORLD, &status);
		MPI_Get_count(&status, MPI_CHAR, &msgSize);

		// Allocate memory to store received message
		char* char_buf = (char*)malloc(sizeof(char) * msgSize);

		// Receive message from head and print to console
		MPI_Recv(char_buf, msgSize, MPI_CHAR, masterRank, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		printf("Rank %d received SEND message '%s'\n", rank, char_buf);
		free(char_buf);
	}

	// Convert pointer to message variable from const void* to void*
	char* buf;
	buf = (char*)newMsg.c_str();

	// Broadcast this to all ranks
	MPI_Bcast(buf, msgSize, MPI_CHAR, masterRank, MPI_COMM_WORLD);
	if (rank != masterRank)
	{
		// Print the broadcast if not from the head
		printf("Rank %d received BCAST message '%s'\n", rank, buf);
	}

	// Finalize the MPI environment
	MPI_Finalize();
}


