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
#define SIZE 1000000

int table[SIZE];

struct ret_struct {
	int left_;
	int right_;
};

struct arg_struct {
	int * table_;
	int id_;
	int pivot_;
	int left_;
	int right_;
};

void generateTable(int table[]) {
	for (int k = 0; k < SIZE; k++)
		table[k] = rand() % SIZE;
}

int generatePivot(int table[], int left_border, int right_border, int size){

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

void QuickSort(int tab[], int left_border, int right_border) {
	
	int size = right_border - left_border + 1;
	if (size < 2) 
		return;

	int left_index = left_border;
	int right_index = right_border;
	int pivot = generatePivot( tab, left_border, right_border, size);

	while (left_index <= right_index) {

		while (table[left_index] < pivot)
			left_index++;

		while (table[right_index] > pivot)
			right_index--;

		if (left_index <= right_index) {

			int mem1 = table[left_index];
			int mem2 = table[right_index];

			table[left_index] = mem2;
			table[right_index] = mem1;

			left_index++;
			right_index--;
		}
	}

	QuickSort(tab, left_border, right_index);
	QuickSort(tab, left_index, right_border);
}

void * swapPass(void * arg) {
	struct arg_struct * arguments = (struct arg_struct *) arg;
	int * tab = arguments->table_;
	int id = arguments->id_;
	int pivot = arguments->pivot_;
	int left_border = arguments->left_;
	int right_border = arguments->right_;

	int left_index = left_border;
	int right_index = right_border;

	while (left_index <= right_index) {

		while (table[left_index] < pivot && left_index <= right_index)
			left_index++;

		while (table[right_index] >= pivot && right_index >= left_index)
			right_index--;


		if (left_index <= right_index) {

			int mem1 = table[left_index];
			int mem2 = table[right_index];

			table[left_index] = mem2;
			table[right_index] = mem1;

			left_index++;
			right_index--;
		}
	}
	 
	return (void *) left_index;
}

void threadQuickSort(int tab[], int temp_tab[], int left_border, int right_border, int proc, int dir) {

	if (proc == 0)
		proc = THREAD_NUM;

	if (proc == 1)
		QuickSort(tab, left_border, right_border);
	else {

		int size = right_border - left_border + 1;
		int pivot = generatePivot(tab, left_border, right_border, size);

		int step = size / proc;
		int rem = size % proc;


		int * starts = (int *)malloc(proc * sizeof(int));
		int * ends = (int *)malloc(proc * sizeof(int));

		// SET BORDERS 
		for (int k = 0; k < proc; k++) {

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

		for (int k = 0; k < proc; k++) {
			arguments[k].table_ = tab;
			arguments[k].id_ = k;
			arguments[k].pivot_ = pivot;
			arguments[k].left_ = starts[k];
			arguments[k].right_ = ends[k];

			pthread_create(&thread[k], NULL, swapPass, (void *)&arguments[k]);
		}

		void * ret;
		int * split = (int *)malloc(proc * sizeof(int));

		for (int k = 0; k < proc; k++) {
			pthread_join(thread[k], &ret);
			split[k] = (int)ret;
		}

		// MERGE START
		for (int k = 0; k < SIZE; k++)
			temp_tab[k] = tab[k];

		int counter = 0;
		for (int k = 0; k < proc; k++) {
			for (int i = 0; i < split[k] - starts[k]; i++) {
				tab[counter] = temp_tab[starts[k] + i];
				counter++;
			}
		}

		int splitter = counter;

		for (int k = 0; k < proc; k++) {
			for (int i = 0; i < ends[k] - split[k] + 1; i++) {
				tab[counter] = temp_tab[split[k] + i];
				counter++;
			}
		}

		int size_left = splitter - left_border;
		int size_right = right_border - splitter + 1;

		int proc_left = (int)round((float)(((float)size_left / (float)size) * proc));

		if (proc_left == 0) proc_left = 1;
		else if (proc_left >= proc) proc_left = proc - 1;

		int proc_right = proc - proc_left;

		if (size_left == 0) {
			threadQuickSort(tab, temp_tab, splitter, right_border, proc, 0);
		} else if (size_right == 0) {
		threadQuickSort(tab, temp_tab, left_border, splitter - 1, proc, 0);
		} else {
			threadQuickSort(tab, temp_tab, left_border, splitter - 1, proc_left, 0);
			threadQuickSort(tab, temp_tab, splitter, right_border, proc_right, 0);
		}
	}
}

int main(void) {

	srand(time(NULL));
	generateTable(table);

	clock_t t1, t2;
	t1 = clock();

	int temp_table[SIZE];
	threadQuickSort(table, temp_table, 0, SIZE - 1, 0, 0);

	t2 = clock();
	printf("\n%d-Thread QuickSort Time = %d ms\n", THREAD_NUM, (int)(t2 - t1));

	int check = 1;
	for (int k = 0; k < SIZE - 1; k++)
		if (table[k] > table[k + 1]) check = 0;
	printf("IS SORTED = %d\n", check);

	getc(stdin);

	return 0;
}
