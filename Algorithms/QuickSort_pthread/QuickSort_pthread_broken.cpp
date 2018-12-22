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
#define SIZE 500000
#define REPEAT 10

struct arg_struct { // STRUCT FOR PASSING PARAMETERS TO P_THREAD
	int pivot_;
	int left_;
	int right_;
	int id_;
};

int table[SIZE];
int temp_table[SIZE];

int ret[THREAD_NUM];
int thread_start = 0;
int thread_finish = 0;

pthread_t thread[THREAD_NUM];
struct arg_struct arguments[THREAD_NUM];

int compare(const void * a, const void * b) { // FUNCTION FOR DEFAULT QS 
	return (*(int*)a - *(int*)b);
}

void generateTable() { // INSERT RANDOM INTEGERS IN TABLE TO SORT
	for (int k = 0; k < SIZE; k++)
		table[k] = rand() % SIZE;
}

int generatePivot(int left_border, int right_border, int size) { // GET PIVOT FOR PARTITION

	float pivot = 0;

	if (size > 15) {
		pivot += table[rand() % size + left_border];
		pivot += table[rand() % size + left_border];
		pivot += table[rand() % size + left_border];
		pivot /= 3;
		return (int)round(pivot);
	}
	else {
		pivot = table[rand() % size + left_border];
		return (int)pivot;
	}
}

void QuickSort( int left_border, int right_border) {

	int size = right_border - left_border + 1;
	if (size < 2) return;

	int pivot = generatePivot(left_border, right_border, size);

	int left_index = left_border - 1;
	int right_index = right_border + 1;

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

	QuickSort( left_border, right_index);
	QuickSort( left_index, right_border);
}

void * finalQuickSortStarter(void * arg) {

	struct arg_struct * arguments = (struct arg_struct *) arg;

	int left_border = arguments->left_;
	int right_border = arguments->right_;

	QuickSort(left_border, right_border);
	thread_finish++;
	return NULL;
}

void * swapPass(void * arg) { 

	int id = (int) arg;

	int pivot = arguments[id].pivot_;
	int left_border = arguments[id].left_;
	int right_border = arguments[id].right_;

	int left_index = left_border - 1;
	int right_index = right_border + 1;

	while (left_index <= right_index) {

		left_index++;
		while (table[left_index] <= pivot && left_index <= right_border)
			left_index++;

		right_index--;
		while (table[right_index] > pivot && right_index >= left_border)
			right_index--;

		if (left_index < right_index) {
			int mem = table[left_index];
			table[left_index] = table[right_index];
			table[right_index] = mem;
		}
	}

	ret[id] = right_index + 1;
	return NULL;
}

void threadQuickSort(int left_border, int right_border, int proc, int proc_start) { 

	if (proc == 0)
		proc = THREAD_NUM;

	if (proc == 1) {
		int id = thread_start;
		thread_start++;
		
		arguments[id].left_ = left_border;
		arguments[id].right_ = right_border;

		/* POSKUSU POPRAVIT TO Z SIGNALOM 11 VENDAR NE LIH DELUJE*/
		while (11 == pthread_create(&thread[id], NULL, finalQuickSortStarter, (void *)&arguments[id])) {}
		pthread_detach(thread[id]);

	} else {

		int size = right_border - left_border + 1;
		int pivot = generatePivot(left_border, right_border, size);

		int step = size / proc;
		int rem = size % proc;

		int * starts = (int *)malloc(proc * sizeof(int));
		int * ends = (int *)malloc(proc * sizeof(int));

		for (int k = 0; k < proc; k++) {

			if (k == 0) starts[k] = left_border;
			else starts[k] = ends[k - 1] + 1;

			ends[k] = starts[k] + step - 1;

			if (rem > 0) {
				rem--;
				ends[k]++;
			}
		}

		for (int k = proc_start; k < proc_start + proc; k++) {
			ret[k] = -1;

			arguments[k].pivot_ = pivot;
			arguments[k].left_ = starts[k];
			arguments[k].right_ = ends[k];

			while (11 == pthread_create(&thread[k], NULL, swapPass, (void *)k)) {}
			pthread_detach(thread[k]);
		}

		int * split = (int *)malloc(proc * sizeof(int));

		for (int k = proc_start; k < proc_start + proc; k++) {
			while (ret[k] == -1) {}
			split[k - proc_start] = ret[k];
			ret[k] = -1;
		}

		for (int k = left_border; k <= right_border; k++)
			temp_table[k] = table[k];

		int offset = left_border;

		for (int k = 0; k < proc; k++) {
			for (int i = starts[k]; i < split[k]; i++) {
				table[offset] = temp_table[i];
				offset++;
			}
		}

		int splitter = offset;

		for (int k = 0; k < proc; k++) {
			for (int i = split[k]; i <= ends[k]; i++) {
				table[offset] = temp_table[i];
				offset++;
			}
		}

		int size_left = splitter - left_border;
		int size_right = right_border - splitter + 1;

		int proc_left = round(((float)size_left / (float)size) * proc);

		if (proc_left <= 0)	proc_left = 1;
		else if (proc_left >= proc) proc_left = proc - 1;

		int proc_right = proc - proc_left;

		if (proc_left > 0 ) threadQuickSort( left_border, splitter - 1, proc_left, 0);
		if (proc_right > 0) threadQuickSort( splitter, right_border, proc_right, proc_left);
	}

	if (proc == THREAD_NUM) {
		while (thread_finish < THREAD_NUM) {}
		thread_finish = 0;
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
		generateTable();
		for (int k = 0; k < SIZE; k++) sol[k] = table[k];
		//

		// THREADED QS
		t1 = clock();
		threadQuickSort(0, SIZE - 1, 0, 0);
		t2 = clock();

		// DEFAULT QS
		t3 = clock();
		qsort(sol, SIZE, sizeof(int), compare);
		t4 = clock();

		avg_time += (int)(t2 - t1);
		avg_dtime += (int)(t4 - t3);

		// CHECK IF SORTED
		sorted = 1;
		for (int k = 0; k < SIZE; k++)
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
