__kernel void sort(__global int *work_array, int size, int workgroup_size) {

	printf("%d\n", get_global_id(0));

	int g_id = get_group_id(0);

	int start = g_id * workgroup_size;
	int end = start + workgroup_size;

	for (int k = start; k < end; k++) {
		work_array[k] = k;
	}

}