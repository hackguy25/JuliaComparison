#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include "FreeImage.h"
#include <CL/cl.h>

#define WORKGROUP_SIZE 8
#define MAX_SOURCE_SIZE	20000

int main(void) {

	char * file_name = "image_512.png";
	//char * file_name = "image_1024.png";
	//char * file_name = "image_2048.png";
	//char * file_name = "image_4096.png";

	FIBITMAP *imageBitmap = FreeImage_Load(FIF_PNG, file_name, 0);
	FIBITMAP *imageBitmapGrey = FreeImage_ConvertToGreyscale(imageBitmap);

	int width = FreeImage_GetWidth(imageBitmapGrey);
	int height = FreeImage_GetHeight(imageBitmapGrey);
	int pitch = FreeImage_GetPitch(imageBitmapGrey);

	printf("IMAGE_RESOLUTION: %dx%d\n", width, height);

	unsigned char *image_in = (unsigned char*)malloc(height*width * sizeof(unsigned char));
	unsigned char *image_out = (unsigned char*)malloc(height*width * sizeof(unsigned char));

	FreeImage_ConvertToRawBits(image_in, imageBitmapGrey, pitch, 8, 0xFF, 0xFF, 0xFF, TRUE);
	FreeImage_Unload(imageBitmapGrey);
	FreeImage_Unload(imageBitmap);

	unsigned int quantization_matrix[64] = {
		16, 11, 10, 16, 24, 40, 51, 61,
		12, 12, 14, 19, 26, 58, 60, 55,
		14, 13, 16, 24, 40, 57, 69, 56,
		14, 17, 22, 29, 51, 87, 80, 62,
		18, 22, 37, 56, 68,109,103, 77,
		24, 35, 55, 64, 81,104,113, 92,
		49, 64, 78, 87,103,121,120,101,
		72, 92, 95, 98,112,100,103, 99
	}; // WARNING !! 1D ARRAY - NOT 2D

	clock_t t1, t2;
	printf("GPU start...\n");
	t1 = clock();

	// GPU CODE

	cl_int ret;

	FILE *fp;
	char *source_str;
	size_t source_size;

	fp = fopen("jpeg_kernel.cl", "r");
	if (!fp)
	{
		fprintf(stderr, ":-(#\n");
		exit(1);
	}
	source_str = (char*)malloc(MAX_SOURCE_SIZE);
	source_size = fread(source_str, 1, MAX_SOURCE_SIZE, fp);
	source_str[source_size] = '\0';
	fclose(fp);

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

	cl_mem imagein_mem_obj = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, height * width * sizeof(unsigned char), image_in, &ret);
	cl_mem qmatrix_mem_obj = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, 64 * sizeof(unsigned int), quantization_matrix, &ret);
	cl_mem imageout_mem_obj = clCreateBuffer(context, CL_MEM_WRITE_ONLY, height * width * sizeof(unsigned char), NULL, &ret);

	cl_program program = clCreateProgramWithSource(context, 1, (const char **)&source_str, NULL, &ret);

	ret = clBuildProgram(program, 1, &device_id[0], NULL, NULL, NULL);

	size_t build_log_len;
	char *build_log;
	ret = clGetProgramBuildInfo(program, device_id[0], CL_PROGRAM_BUILD_LOG, 0, NULL, &build_log_len);
	build_log = (char *)malloc(sizeof(char)*(build_log_len + 1));
	ret = clGetProgramBuildInfo(program, device_id[0], CL_PROGRAM_BUILD_LOG,
		build_log_len, build_log, NULL);
	printf("%s\n", build_log);
	free(build_log);

	cl_kernel kernel = clCreateKernel(program, "jpeg", &ret);

	size_t buf_size_t;
	clGetKernelWorkGroupInfo(kernel, device_id[0], CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE, sizeof(buf_size_t), &buf_size_t, NULL);

	ret = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&imagein_mem_obj);
	ret |= clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *)&imageout_mem_obj);
	ret |= clSetKernelArg(kernel, 2, sizeof(cl_mem), (void *)&qmatrix_mem_obj);
	ret |= clSetKernelArg(kernel, 3, sizeof(cl_int), (void *)&width);
	ret |= clSetKernelArg(kernel, 4, sizeof(cl_int), (void *)&height);


	size_t global_item_size[2] = { width, height };
	size_t local_item_size[2] = { WORKGROUP_SIZE, WORKGROUP_SIZE };

	ret = clEnqueueNDRangeKernel(command_queue, kernel, 2, NULL, &global_item_size, &local_item_size, 0, NULL, NULL);
	ret = clEnqueueReadBuffer(command_queue, imageout_mem_obj, CL_TRUE, 0, height * width * sizeof(unsigned char), image_out, 0, NULL, NULL);

	ret = clFlush(command_queue);
	ret = clFinish(command_queue);
	ret = clReleaseKernel(kernel);
	ret = clReleaseProgram(program);
	ret = clReleaseMemObject(imagein_mem_obj);
	ret = clReleaseMemObject(qmatrix_mem_obj);
	ret = clReleaseMemObject(imageout_mem_obj);
	ret = clReleaseCommandQueue(command_queue);
	ret = clReleaseContext(context);

	// GPU END

	t2 = clock();
	double time2 = (double)t2 - (double)t1;

	printf("GPU time: %d\n", (int)time2);

	FIBITMAP *imageOutBitmapGPU = FreeImage_ConvertFromRawBits(image_out, width, height, pitch, 8, 0xFF, 0xFF, 0xFF, TRUE);
	FreeImage_Save(FIF_PNG, imageOutBitmapGPU, "image_compressed.png", 0);
	FreeImage_Unload(imageOutBitmapGPU);

	char c;
	scanf("Press any key to terminate program: %c", &c);

	return 0;
}

