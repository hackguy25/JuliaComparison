#define WORKGROUP_SIZE 8
#define PI 3.141592

__kernel void jpeg(__global unsigned char *imageIn, __global unsigned char *imageOut, __global unsigned int *q_matrix, int width, int height) {

	int l_id0 = get_local_id(0);
	int l_id1 = get_local_id(1);

	int g_id0 = get_group_id(0);
	int g_id1 = get_group_id(1);

	int t_y = get_global_id(0);
	int t_x = get_global_id(1);

	__local float idct[WORKGROUP_SIZE][WORKGROUP_SIZE];
	__local float dct[WORKGROUP_SIZE][WORKGROUP_SIZE];
	__local unsigned char local_image[WORKGROUP_SIZE][WORKGROUP_SIZE];
	__local unsigned int quantization_matrix[WORKGROUP_SIZE][WORKGROUP_SIZE];

	local_image[l_id0][l_id1] = imageIn[t_y * width + t_x];
	quantization_matrix[l_id0][l_id1] = q_matrix[l_id0 * 8 + l_id1];

	/* WAIT FOR ALL THREADS TO WRITE IN LOCAL MEMORY*/ barrier(CLK_LOCAL_MEM_FENCE);

	float sum = 0;
	for (int n = 0; n < 8; n++)
		sum += local_image[l_id0][n] * cos((PI / 8)*(n + 0.5)* l_id1);
	dct[l_id0][l_id1] = sum;

	/* WAIT FOR ALL THREADS TO CALCULATE 1D DCT*/ barrier(CLK_LOCAL_MEM_FENCE);

	sum = 0;
	for (int n = 0; n < 8; n++)
		sum += dct[n][l_id1] * cos((PI / 8)*(n + 0.5)* l_id0);
	dct[l_id0][l_id1] = sum;

	/* WAIT FOR ALL THREADS TO CALCULATE 2D DCT*/ barrier(CLK_LOCAL_MEM_FENCE);

	dct[l_id0][l_id1] = dct[l_id0][l_id1] / quantization_matrix[l_id0][l_id1];
	dct[l_id0][l_id1] = round(dct[l_id0][l_id1]);
	dct[l_id0][l_id1] = dct[l_id0][l_id1] * quantization_matrix[l_id0][l_id1];

	/* WAIT FOR ALL THREADS TO DIVIDE DCT WITH Q_MATRIX AND ROUND AND MULTIPLY*/ barrier(CLK_LOCAL_MEM_FENCE);

	sum = 0.125 * dct[0][l_id1];
	for (int n = 1; n < 8; n++)
		sum += 0.25 * dct[n][l_id1] * cos((PI / 8)* n * (l_id0 + 0.5));
	idct[l_id0][l_id1] = sum;

	/* WAIT FOR ALL THREADS TO CALCULATE 1D inverse-DCT*/ barrier(CLK_LOCAL_MEM_FENCE);

	sum = 0.125 * idct[l_id0][0];
	for (int n = 1; n < 8; n++)
		sum += 0.25 * idct[l_id0][n] * cos((PI / 8)* n * (l_id1 + 0.5));
	idct[l_id0][l_id1] = round(sum);

	/* WAIT FOR ALL THREADS TO CALCULATE 2D inverse-DCT*/ barrier(CLK_LOCAL_MEM_FENCE);

	imageOut[t_y * width + t_x] = idct[l_id0][l_id1];
}