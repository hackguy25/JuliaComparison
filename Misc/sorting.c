#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <CL/cl.h>


#define ARRAY_SIZE 4000
#define WORKGROUP_SIZE 8
#define MAX_SOURCE_SIZE	20000

void main() {

	int workgroup_size = WORKGROUP_SIZE;
	int size = ARRAY_SIZE;
	int work_array[ARRAY_SIZE];

	for (int k = 0; k < ARRAY_SIZE; k++)
		work_array[k] = (int) rand();

	FILE *fp;
	char *source_str;
	size_t source_size;

	fp = fopen("kernel_sorting.cl", "r");
	if (!fp)
	{
		fprintf(stderr, ":-(#\n");
		exit(1);
	}
	source_str = (char*)malloc(MAX_SOURCE_SIZE);
	source_size = fread(source_str, 1, MAX_SOURCE_SIZE, fp);
	source_str[source_size] = '\0';
	fclose(fp);

	cl_int ret;
	cl_platform_id	platform_id[10];
	cl_uint			ret_num_platforms;
	char			*buf;
	size_t			buf_len;

	ret = clGetPlatformIDs(10, platform_id, &ret_num_platforms);

	cl_device_id	device_id[10];
	cl_uint			ret_num_devices;

	ret = clGetDeviceIDs(platform_id[2], CL_DEVICE_TYPE_GPU, 10, device_id, &ret_num_devices);

	cl_context context = clCreateContext(NULL, 1, &device_id[0], NULL, NULL, &ret);
	cl_command_queue command_queue = clCreateCommandQueue(context, device_id[0], 0, &ret);

	cl_mem array_mem_obj = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, ARRAY_SIZE * sizeof(int), work_array, &ret);

	cl_program program = clCreateProgramWithSource(context, 1, (const char **)&source_str, NULL, &ret);

	ret = clBuildProgram(program, 1, &device_id[0], NULL, NULL, NULL);

	size_t build_log_len;
	char *build_log;
	ret = clGetProgramBuildInfo(program, device_id[0], CL_PROGRAM_BUILD_LOG, 0, NULL, &build_log_len);
	build_log = (char *)malloc(sizeof(char)*(build_log_len + 1));
	ret = clGetProgramBuildInfo(program, device_id[0], CL_PROGRAM_BUILD_LOG, build_log_len, build_log, NULL);
	printf("%s\n", build_log);
	free(build_log);

	cl_kernel kernel = clCreateKernel(program, "sort", &ret);

	size_t buf_size_t;
	clGetKernelWorkGroupInfo(kernel, device_id[0], CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE, sizeof(buf_size_t), &buf_size_t, NULL);

	char ch;
	scanf("%c", &ch);

	ret  = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *) &array_mem_obj);
	ret |= clSetKernelArg(kernel, 1, sizeof(cl_int), (void *) &size);
	ret |= clSetKernelArg(kernel, 2, sizeof(cl_int), (void *) &workgroup_size);

	size_t global_item_size = { ARRAY_SIZE };
	size_t local_item_size = { WORKGROUP_SIZE };

	ret = clEnqueueNDRangeKernel(command_queue, kernel, 1, NULL, &global_item_size, &local_item_size, 0, NULL, NULL);
	ret = clEnqueueReadBuffer(command_queue, array_mem_obj, CL_TRUE, 0, ARRAY_SIZE * sizeof(int), work_array, 0, NULL, NULL);

	printf("Kernel run OK\n");

	ret = clFlush(command_queue);
	ret = clFinish(command_queue);
	ret = clReleaseKernel(kernel);
	ret = clReleaseProgram(program);
	ret = clReleaseMemObject(array_mem_obj);
	ret = clReleaseCommandQueue(command_queue);
	ret = clReleaseContext(context);

	char c;
	scanf("%c", &c);

	//for (int k = 0; k < size; k++)
	//	printf("%d\n", work_array[k]);


	return 0;
}