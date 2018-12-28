#define HAVE_STRUCT_TIMESPEC

#include <stdio.h>
#include <stdlib.h>  
#include <math.h>
#include <time.h>
#include <windows.h> 
#include <algorithm>
#include "omp.h"

#define THREAD_NUM 8
#define SIZE 500000
#define REPEAT 40
#define SORT_CHECK 1 // 1 - check if sorted --- 0 - ignore

// 

int table[SIZE];
int starts[THREAD_NUM];
int split[THREAD_NUM];
int ends[THREAD_NUM];
int pivot[THREAD_NUM];

int compare(const void * a, const void * b) { // FUNCTION FOR DEFAULT QS 
	return (*(int*)a - *(int*)b);
}

void generateTable() { // INSERT RANDOM INTEGERS IN TABLE TO SORT
	for (int k = 0; k < SIZE; k++)
		table[k] = rand();
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

void QuickSort(int left_border, int right_border) {

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

	QuickSort(left_border, right_index);
	QuickSort(left_index, right_border);
}

void swapPass(int id) {

	int left_border = starts[id];
	int right_border = ends[id];

	int left_index = left_border - 1;
	int right_index = right_border + 1;

	while (left_index <= right_index) {

		left_index++;
		while (table[left_index] <= pivot[id] && left_index <= right_border)
			left_index++;

		right_index--;
		while (table[right_index] > pivot[id] && right_index >= left_border)
			right_index--;

		if (left_index < right_index) {
			int mem = table[left_index];
			table[left_index] = table[right_index];
			table[right_index] = mem;
		}
	}
	split[id] = right_index + 1;
}

void threadQuickSort(int left_border, int right_border, int proc_start, int proc, int init) { // MAIN QS FUNCTION

	if (proc == 0)
		proc = THREAD_NUM;

	if (proc == 1) {
		starts[proc_start] = left_border;
		ends[proc_start] = right_border;
	}
	else {
		int size = right_border - left_border + 1;
		int pivot_ = generatePivot(left_border, right_border, size);

		int step = size / proc;
		int rem = size % proc;

		for (int k = proc_start; k < proc_start + proc; k++) {

			pivot[k] = pivot_;

			if (k == 0) starts[k] = left_border;
			else starts[k] = ends[k - 1] + 1;

			ends[k] = starts[k] + step - 1;

			if (rem > 0) {
				rem--;
				ends[k]++;
			}
		}

		int k;
#pragma omp parallel for private(k)
		for (k = proc_start; k < proc_start + proc; k++)
		{
			swapPass(k);
		}

		int splitter = starts[proc_start];
		int counter = starts[proc_start];

		for (int k = proc_start; k < proc_start + proc; k++) {

			for (int i = starts[k]; i < split[k]; i++) {
				int mem = table[counter];
				table[counter] = table[i];
				table[i] = mem;

				counter++;
				splitter++;
			}
		}

		int size_left = splitter - left_border;
		int size_right = right_border - splitter + 1;

		int proc_left = round(((float)size_left / (float)size) * proc);

		if (proc_left <= 0)	proc_left = 1;
		else if (proc_left >= proc) proc_left = proc - 1;

		int proc_right = proc - proc_left;

		if (proc_left > 0) threadQuickSort(left_border, splitter - 1, proc_start, proc_left, 1);
		if (proc_right > 0) threadQuickSort(splitter, right_border, proc_start + proc_left, proc_right, 1);
	}
	if (init == 0) {
		int k;
#pragma omp parallel for private(k)
		for (k = proc_start; k < proc_start + proc; k++)
		{
			QuickSort(starts[k], ends[k]);
		}
	}
}

int main(void) {

	/* INIT VARIABLES FOR SORT CHECKING*/
	int check_sort[SIZE];
	int sorted = 1;
	int check_enable = SORT_CHECK;
	int num_sorted = 0;

	srand(time(NULL));
	clock_t t1, t2;

	int avg_time = 0;

	printf("QUICKSORT THREADS = %d\nARRAY SIZE = %d\nREPEATS = %d\n\n", THREAD_NUM, SIZE, REPEAT);

	omp_set_num_threads(THREAD_NUM);

	for (int k = 0; k < REPEAT; k++) {

		generateTable();

		if (check_enable) {
			for (int k = 0; k < SIZE; k++)
				check_sort[k] = table[k];
		}

		t1 = clock();
		threadQuickSort(0, SIZE - 1, 0, 0, 0);
		t2 = clock();

		avg_time += (int)(t2 - t1);

		printf("TIME = %d ms\n", (int)(t2 - t1));

		if (check_enable) {

			qsort(check_sort, SIZE, sizeof(int), compare);

			for (int k = 0; k < SIZE; k++)
				if (check_sort[k] != table[k])	sorted = 0;

			printf("IS SORTED = %d\n", sorted);

			if (sorted) num_sorted++;
			sorted = 1;
		}
	}
	avg_time /= REPEAT;

	printf("\nAVERAGE %d-THREAD QUICKSORT TIME = %d ms\n", THREAD_NUM, avg_time);
	if (check_enable) printf("\nSORTED = %d/%d\n", num_sorted, REPEAT);

	getc(stdin); // EMPTY INPUT FOR WAITING

	return 0;
}