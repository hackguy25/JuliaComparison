#define HAVE_STRUCT_TIMESPEC

#include <stdio.h>
#include <stdlib.h>   
#include <pthread.h>
#include <time.h>
#include <windows.h> 
#include <vector>
#include <algorithm>
#include "omp.h"

#define THREAD_NUM 8

#define SIZE 5000000

struct arg_struct {
	int * table_;
	int left_;
	int right_;
};

inline int generatePivot(int table[], int left_border, int right_border){

	int pivot = 0;
	int size = right_border - left_border;

	if (size > 15) {
		pivot += table[rand() % size + left_border];
		pivot += table[rand() % size + left_border];
		pivot += table[rand() % size + left_border];
		pivot /= 3;
		return (int) pivot;
	}
	else {
		pivot = table[rand() % size + left_border];
		return pivot;
	}
}

void * QuickSort(int table[], int left_border, int right_border) {

	if (right_border - left_border < 1) return NULL;

	int pivot = generatePivot(table, left_border, right_border);

	int left_index = left_border;
	int right_index = right_border;

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

	QuickSort(table, left_border, right_index);
	QuickSort(table, left_index, right_border);

	return NULL;
}

void * QuickSort_thread(void * arg) {

	struct arg_struct * arguments = (struct arg_struct *) arg;
	int * table = arguments -> table_;
	int left_border = arguments -> left_;
	int right_border = arguments -> right_;

	if (right_border - left_border < 1) return NULL;
	
	int pivot = generatePivot(table, left_border, right_border);

	int left_index = left_border;
	int right_index = right_border;

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

	QuickSort(table, left_border, right_index);
	QuickSort(table, left_index, right_border);


	return NULL;
}

void normalQuickSort(int table[]) {
	clock_t t1, t2;
	t1 = clock();
	QuickSort(table, 0, SIZE - 1);
	t2 = clock();
	printf("NormalQuickSort Time = %d ms\n", (int)(t2 - t1));

	int bol = 1;
	for (int k = 0; k < SIZE - 1; k++)
		if (table[k] > table[k + 1]) bol = 0;

	printf("Is sorted = %d\n", bol);
}

void threadQuickSort(int table[]) {
	clock_t t1, t2;
	t1 = clock();

	pthread_t thread[THREAD_NUM];
	struct arg_struct arguments[THREAD_NUM];

	for (int k = 0; k < THREAD_NUM; k++) {
		arguments[k].table_ = table;
		arguments[k].left_ = k * (SIZE/THREAD_NUM);
		arguments[k].right_ = ( (k+1) * (SIZE/THREAD_NUM) ) - 1;

		pthread_create(&thread[k], NULL, QuickSort_thread, (void *) &arguments[k]);
	}

	for (int k = 0; k < THREAD_NUM; k++)
		pthread_join(thread[k], NULL);

	
	int table_merge[SIZE];
	int all_counter = 0;
	int mem_pointer = 0;
	int pointer[THREAD_NUM];

	for (int k = 0; k < THREAD_NUM; k++) {
		pointer[k] = k * (SIZE / THREAD_NUM);
	}

	while (all_counter < SIZE) {
		int min = SIZE+1;

		for (int k = 0; k < THREAD_NUM; k++) {
			if (table[pointer[k]] < min && pointer[k] < ((k+1) * (SIZE/THREAD_NUM)) ) {
				min = table[pointer[k]];
				mem_pointer = k;
			}
		}

		table_merge[all_counter] = table[pointer[mem_pointer]];
		pointer[mem_pointer]++;
		all_counter++;
	}

	t2 = clock();
	printf("ThreadQuickSort Time = %d ms\n", (int)(t2 - t1));

	int bol = 1;
	for (int k = 0; k < SIZE - 1; k++)
		if (table_merge[k] > table_merge[k + 1]) bol = 0;

	printf("Is sorted = %d\n", bol);
}

void generateTable(int table[]) {
	for (int k = 0; k < SIZE; k++)
		table[k] = rand() % SIZE;
}

int main(void) {
	
	srand(time(NULL));
	int table[SIZE];
	
	generateTable(table);	
	normalQuickSort(table);

	generateTable(table);
	threadQuickSort(table);

	getc(stdin);
	return 0;
}