#define HAVE_STRUCT_TIMESPEC

#include <stdio.h>
#include <stdlib.h>  
#include <math.h>
#include <time.h>
#include <windows.h> 
#include <algorithm>
#include <pthread.h>
#include "omp.h"

#define THREAD_NUM 2
#define SIZE 10000000
#define REPEAT 3

int table[SIZE];

struct arg_struct { // STRUCT FOR PASSING PARAMETERS TO P_THREAD
	int * table_;
	int pivot_;
	int left_;
	int right_;
};

int compare(const void * a, const void * b) { // FUNCTION FOR DEFAULT QS 
	return (*(int*)a - *(int*)b);
}

void generateTable(int table[]) { // INSERT RANDOM INTEGERS IN TABLE TO SORT
	for (int k = 0; k < SIZE; k++)
		table[k] = rand() % SIZE;
}

int generatePivot(int table[], int left_border, int right_border, int size){ // GET PIVOT FOR PARTITION

	float pivot = 0;

	if (size > 15) {
		pivot += table[rand() % size + left_border];
		pivot += table[rand() % size + left_border];
		pivot += table[rand() % size + left_border];
		pivot /= 3;
		return (int) round(pivot);
	} else {
		pivot = table[rand() % size + left_border];
		return (int) pivot;
	}
}

void QuickSort(int tab[], int left_border, int right_border) { // LAST PARTITIONS QS

	int size = right_border - left_border + 1;
	if (size < 2) return;

	int pivot = generatePivot( tab, left_border, right_border, size);
	
	int left_index = left_border-1;
	int right_index = right_border+1;

	while (left_index <= right_index) {

		left_index++;
		while (table[left_index] < pivot)
			left_index++;

		right_index--;
		while (table[right_index] > pivot)
			right_index--;

		if (left_index < right_index) {
			int mem = table[left_index];
			table[left_index] = table[right_index];
			table[right_index] = mem;
		}
	}

	QuickSort(tab, left_border, right_index);
	QuickSort(tab, left_index, right_border);
}

void * swapPass(void * arg) { // ONE SWAP PASS THROUGH PARTITION

	struct arg_struct * arguments = (struct arg_struct *) arg;
	
	int * tab = arguments->table_; // EXTRACTING ARGUMENTS
	int pivot = arguments->pivot_;
	int left_border = arguments->left_;
	int right_border = arguments->right_;

	int left_index = left_border - 1; 
	int right_index = right_border + 1;

	while (left_index <= right_index) {

		left_index++;
		while (table[left_index] <= pivot && left_index <= right_border)
			left_index++;

		right_index--;
		while (table[right_index] > pivot && right_index >= left_border)
			right_index--;

		if (left_index < right_index) { // SWAP
			int mem = table[left_index];
			table[left_index] = table[right_index];
			table[right_index] = mem;
		}
	}
	return (void *) right_index; // RETURN SPLIT POINT
}

void threadQuickSort(int tab[], int temp_tab[], int left_border, int right_border, int proc, int dir) { // MAIN QS FUNCTION

	if (proc == 0)
		proc = THREAD_NUM;

	if (proc == 1) {
		QuickSort(tab, left_border, right_border);
	} else {
		int size = right_border - left_border + 1;
		int pivot = generatePivot(tab, left_border, right_border, size);

		int step = size / proc;
		int rem = size % proc;

		int * starts = (int *)malloc(proc * sizeof(int));
		int * ends = (int *)malloc(proc * sizeof(int));

		for (int k = 0; k < proc; k++) { // SET PARTITION START AND END

			if (k == 0) starts[k] = left_border;
			else starts[k] = ends[k - 1] + 1;

			ends[k] = starts[k] + step - 1;

			if (rem > 0) {
				rem--;
				ends[k]++;
			}
		}

		// RUN SWAP_PASS ON EACH PART
		pthread_t * thread = (pthread_t *)malloc(proc * sizeof(pthread_t));
		struct arg_struct * arguments = (arg_struct *)malloc(proc * sizeof(arg_struct));
		void ** ret = (void **)malloc(proc * sizeof(void *));
		int * split = (int *)malloc(proc * sizeof(int));

		for (int k = 0; k < proc; k++) {
			arguments[k].table_ = tab;
			arguments[k].pivot_ = pivot;
			arguments[k].left_ = starts[k];
			arguments[k].right_ = ends[k];

			// SPAWN THREADS AND COLLECT RESULTS
			pthread_create(&thread[k], NULL, swapPass, (void *)&arguments[k]);
			pthread_join(thread[k], &ret[k]);
			
			split[k] = (int)ret[k] + 1;
		}

		for (int k = left_border; k <= right_border; k++) // CROSOVER PART START
			temp_tab[k] = tab[k];

		int counter = 0;
		for (int k = 0; k < proc; k++) {
			for (int i = left_border; i < split[k] - starts[k]; i++) {
				tab[counter] = temp_tab[starts[k] + i];
				counter++;
			}
			for (int i = left_border; i < ends[k] - split[k] + 1; i++) {
				tab[counter] = temp_tab[split[k] + i];
				counter++;
			}
		}

		int splitter = left_border + counter; // DETERMINE NUMBER OF THREADS PER PARTITION

		int size_left = splitter - left_border;
		int size_right = right_border - splitter + 1;

		int proc_left = round(((float)size_left / (float)size) * proc);

		if		(proc_left <= 0)	proc_left = 1;
		else if (proc_left >= proc) proc_left = proc - 1;

		int proc_right = proc - proc_left;

		if( proc_left  > 0 ) threadQuickSort( tab, temp_tab, left_border, splitter-1, proc_left, 0);
		if( proc_right > 0 ) threadQuickSort( tab, temp_tab, splitter, right_border, proc_right, 0);
	}
}

int main(void) {

	srand(time(NULL));
	clock_t t1, t2, t3, t4;

	int n = REPEAT; // NUMBER OF REPETITIONS
	int avg_time = 0;
	int avg_dtime = 0;
	int sorted = 0;
	int num_sorted = 0;
	int sol[SIZE];

	printf("QUICKSORT THREADS = %d\nARRAY SIZE = %d\nREPEATS = %d\n\n", THREAD_NUM, SIZE, n);

	for (int k = 0; k < n; k++) {

		// INITIALITE TEMP TABLE, COPY TABLE
		int temp_table[SIZE];
		generateTable(table);
		for (int k = 0; k < SIZE; k++) sol[k] = table[k];
		//

		// THREADED QS
		t1 = clock();
		threadQuickSort(table, temp_table, 0, SIZE - 1, 0, 0);
		t2 = clock();

		// DEFAULT QS
		t3 = clock();
		qsort(sol, SIZE, sizeof(int), compare);
		t4 = clock();

		avg_time += (int)(t2 - t1);
		avg_dtime += (int)(t4 - t3);

		// CHECK IF SORTED
		sorted = 1;
		for (int k = 0; k < SIZE ; k++)
			if (sol[k] != table[k])	sorted = 0;

		printf("IS SORTED = %d --- TIME = %d ms\n", sorted, (int)(t2 - t1));
		if (sorted) num_sorted++;
		//
	}
	avg_time /= n;
	avg_dtime /= n;
	printf("\n\nSORTED = %d / %d ---\nAVERAGE THREAD-QS TIME = %d ms\nAVERAGE DEFAULT-QS TIME = %d ms\n", num_sorted, n, avg_time, avg_dtime);

	getc(stdin); // EMPTY INPUT FOR WAITING

	return 0;
}
