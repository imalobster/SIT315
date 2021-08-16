#include <iostream>
#include <cstdlib>
#include <time.h>
#include <chrono>
#include <omp.h>

using namespace std::chrono;
using namespace std;

const int NUM_THREADS = 24;

void randomVector(int vector[], int size)
{
	#pragma omp parallel default(none) shared(vector) firstprivate(size)
	{
		// Generate random values between 0 and 99 for the partition 
		#pragma omp for
		for (int i = 0; i < size; i++)
		{
			vector[i] = rand() % 100;
		}
		//printf("thread %d: size = %d\n", omp_get_thread_num(), size);
	}
}

int main(){

	unsigned long size = 100000000;

	srand(time(0));

	int *v1, *v2, *v3;

	// Get the current time before vector assignment
	auto start = high_resolution_clock::now();

	// Allocate three lots of contiguous memory of a set size and return the pointer to the first location
	v1 = (int *) malloc(size * sizeof(int *));
	v2 = (int *) malloc(size * sizeof(int *));
	v3 = (int *) malloc(size * sizeof(int *));

	omp_set_num_threads(NUM_THREADS);

	// Use the custom function to generate random values in the first and second blocks of memory
	randomVector(v1, size);
	randomVector(v2, size);
	
	// 1. Standard parallel directive
	#pragma omp parallel default(none) shared(v1, v2, v3) firstprivate(size)
	{
		// Assign values to v3 based on the sum of values from v1 and v2
		#pragma omp for
		for (int i = 0; i < size; i++)
		{
			v3[i] = v1[i] + v2[i];
		}
	}

	// 2. Atomic update directive
	unsigned long totalAtomic = 0;
	auto startAtomic = high_resolution_clock::now();
	#pragma omp parallel default(none) shared(totalAtomic, v3) firstprivate(size)
	{
		#pragma omp for
		for(int i = 0; i < size; i++)
		{
			#pragma omp atomic update
			totalAtomic += v3[i];
		}
	}

	// 3. Reduction clause
	unsigned long totalReduction = 0;
	auto startReduction = high_resolution_clock::now();
	#pragma omp parallel reduction(+:totalReduction)
	{
		#pragma omp for
		for(int i = 0; i < size; i++)
		{
			totalReduction += v3[i];
		}
	}

	// 4. Critial directive
	unsigned long totalCritical = 0;
	auto startCritical = high_resolution_clock::now();
	#pragma omp parallel
	{
		unsigned long localSum = 0;

		#pragma omp for
		for(int i = 0; i < size; i++)
		{
			localSum += v3[i];
		}
		#pragma omp critical
		totalCritical += localSum;
	}

	// 5. Scheduling techniques
	// Static
	auto startScheduleStatic = high_resolution_clock::now();
	#pragma omp parallel default(none) shared(v1, v2, v3) firstprivate(size)
	{
		// Assign values to v3 based on the sum of values from v1 and v2
		#pragma omp for schedule(static, 100)
		for (int i = 0; i < size; i++)
		{
			v3[i] = v1[i] + v2[i];
		}
	}

	// Dynamic
	auto startScheduleDynamic = high_resolution_clock::now();
	#pragma omp parallel default(none) shared(v1, v2, v3) firstprivate(size)
	{
		// Assign values to v3 based on the sum of values from v1 and v2
		#pragma omp for schedule(dynamic, 100)
		for (int i = 0; i < size; i++)
		{
			v3[i] = v1[i] + v2[i];
		}
	}

	// Guided
	auto startScheduleGuided = high_resolution_clock::now();
	#pragma omp parallel default(none) shared(v1, v2, v3) firstprivate(size)
	{
		// Assign values to v3 based on the sum of values from v1 and v2
		#pragma omp for schedule(guided, 100)
		for (int i = 0; i < size; i++)
		{
			v3[i] = v1[i] + v2[i];
		}
	}

	// Auto
	auto startScheduleAuto = high_resolution_clock::now();
	#pragma omp parallel default(none) shared(v1, v2, v3) firstprivate(size)
	{
		// Assign values to v3 based on the sum of values from v1 and v2
		#pragma omp for schedule(auto)
		for (int i = 0; i < size; i++)
		{
			v3[i] = v1[i] + v2[i];
		}
	}

	// Get the current time after all operations
	auto stop = high_resolution_clock::now();

	// Verify totals are equal
	bool totalCheck = totalAtomic == totalReduction && totalReduction == totalCritical;

	// Obtain the difference between start and stop times, then cast to microseconds format
	auto duration = duration_cast<microseconds>(stop - start);
	auto durationAtomic = duration_cast<microseconds>(startReduction - startAtomic);
	auto durationReduction = duration_cast<microseconds>(startCritical - startReduction);
	auto durationCritical = duration_cast<microseconds>(startScheduleStatic - startCritical);
	auto durationScheduleStatic = duration_cast<microseconds>(startScheduleDynamic - startScheduleStatic);
	auto durationScheduleDynamic = duration_cast<microseconds>(startScheduleGuided - startScheduleDynamic);
	auto durationScheduleGuided = duration_cast<microseconds>(startScheduleAuto - startScheduleGuided);
	auto durationScheduleAuto = duration_cast<microseconds>(stop - startScheduleAuto);

	// Print results
	cout << "Total time taken: "
		<< duration.count() << " microseconds" << endl
		<< "2. Atomic update time: " << durationAtomic.count() << endl 
		<< "3. Reduction clause time: " << durationReduction.count() << endl
		<< "4. Critical section time: " << durationCritical.count() << endl
		<< "Are totals equal: " << totalCheck << endl
		<< "5.1. Static schedule time: " << durationScheduleStatic.count() << endl
		<< "5.2. Dynamic schedule time: " << durationScheduleDynamic.count() << endl
		<< "5.3. Guided schedule time: " << durationScheduleGuided.count() << endl
		<< "5.4. Auto schedule time: " << durationScheduleAuto.count() << endl;

	return 0;
}