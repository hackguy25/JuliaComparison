#define HAVE_STRUCT_TIMESPEC

#include <stdio.h>
#include <stdlib.h> 
#include <math.h>
#include <time.h>
#include <algorithm>
#include <pthread.h>

#define THREAD_NUM 8
#define SIZE 2000000
#define REPEAT 10
#define SORT_CHECK 0 // 1 - check if sorted --- 0 - not checking

long table[SIZE];
long pivot[THREAD_NUM];

int starts[THREAD_NUM];
int split[THREAD_NUM];
int ends[THREAD_NUM];

pthread_t thread[THREAD_NUM];

unsigned long seed;

unsigned long rand_ulong(void) {
	unsigned long x = seed;
	x ^= x << 13;
	x ^= x >> 17;
	x ^= x << 5;
	seed = x;
	return x;
}

int compare(const void * a, const void * b) { // FUNCTION FOR DEFAULT QS 
	if (*(long*)a - *(long*)b < 0)
		return -1;
	if (*(long*)a - *(long*)b > 0)
		return 1;
	return 0;
}

void generateTable() { // INSERT RANDOM INTEGERS IN TABLE TO SORT
	int sign;
	for (int k = 0; k < SIZE; k++) {
		sign = 1;
		if (rand_ulong() % 2 == 0) sign = -1;
		table[k] = sign * rand_ulong() % 2000000000;
	}
}

long generatePivot(int left_border, int right_border, int size) { // GET PIVOT FOR PARTITION

	long pivot = 0;

	if (size > 15) {
		pivot += table[ rand_ulong() % size + left_border]/3;
		pivot += table[ rand_ulong() % size + left_border]/3;
		pivot += table[ rand_ulong() % size + left_border]/3;
		return pivot;
	} else {
		pivot = table[ rand_ulong() % size + left_border];
		return pivot;
	}
}

void QuickSort(int left_border, int right_border) {

	int size = right_border - left_border + 1;
	if (size < 2) return;

	long pivot = generatePivot(left_border, right_border, size);

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
			long mem = table[left_index];
			table[left_index] = table[right_index];
			table[right_index] = mem;
		}
	}

	QuickSort(left_border, right_index);
	QuickSort(left_index, right_border);
}

void * finalQuickSortStarter(void * arg) {

	int id = (int)arg;

	int left_border = starts[id];
	int right_border = ends[id];

	QuickSort(left_border, right_border);

	return NULL;
}

void * swapPass(void * arg) {

	int id = (int)arg;

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
			long mem = table[left_index];
			table[left_index] = table[right_index];
			table[right_index] = mem;
		}
	}
	split[id] = right_index + 1;

	return NULL;
}

void threadQuickSort(int left_border, int right_border, int proc_start, int proc, int init) { // MAIN QS FUNCTION

	if (proc == 0)
		proc = THREAD_NUM;

	if (proc == 1) {
		starts[proc_start] = left_border;
		ends[proc_start] = right_border;
	} else {
		int size = right_border - left_border + 1;
		long pivot_ = generatePivot(left_border, right_border, size);

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

		for (int k = proc_start; k < proc_start + proc; k++) 
			pthread_create(&thread[k], NULL, swapPass, (void *)k);
		
		for (int k = proc_start; k < proc_start + proc; k++) 
			pthread_join(thread[k], NULL);
		
		int splitter = starts[proc_start];
		int counter = starts[proc_start];

		for (int k = proc_start; k < proc_start + proc; k++) {
			for (int i = starts[k]; i < split[k]; i++) {
				long mem = table[counter];
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
		for (int k = 0; k < THREAD_NUM; k++)
			pthread_create(&thread[k], NULL, finalQuickSortStarter, (void *)k);

		for (int k = 0; k < THREAD_NUM; k++)
			pthread_join(thread[k], NULL);
	}
}

int main(void) {

	long check_sort[SIZE];
	int sorted = 1;
	int num_sorted = 0;

	seed = (unsigned long) time(NULL);
	srand(time(NULL));

	clock_t t1, t2;

	long avg_time = 0;

	printf("QUICKSORT THREADS = %d\nARRAY SIZE = %d\nREPEATS = %d\n\n", THREAD_NUM, SIZE, REPEAT);

	for (int k = 0; k < REPEAT; k++) {

		generateTable();

		if( SORT_CHECK ) {
			for (int k = 0; k < SIZE; k++)
				check_sort[k] = table[k];
		}

		t1 = clock();
		threadQuickSort(0, SIZE - 1, 0, 0, 0);
		t2 = clock();

		avg_time += (long)(t2 - t1);

		printf("TIME = %ld ms\n", (long)(t2 - t1));

		if ( SORT_CHECK ) {

			qsort(check_sort, SIZE, sizeof(long), compare);

			for (int k = 0; k < SIZE; k++) {
				if (check_sort[k] != table[k]) {
					sorted = 0;
					printf("DIFF AT %d\n", k);
				}
				break;
			}
			printf("IS SORTED = %d\n", sorted);

			if (sorted) num_sorted++;
			sorted = 1;
		}
	}
	avg_time /= REPEAT;

	printf("\nAVERAGE %d-THREAD QUICKSORT TIME = %ld ms\n", THREAD_NUM, avg_time);
	if( SORT_CHECK ) printf("\nSORTED = %d/%d\n", num_sorted, REPEAT);

	getc(stdin); // EMPTY INPUT FOR WAITING

	return 0;
}
