#include "openCL.h"

/*=====================================================*/
/*=====================================================*/
/*================= ADDITIONAL FUNCTIONS ==============*/
/*=====================================================*/
/*=====================================================*/

int* compressImage(const unsigned char* a, const int& width, const int& height) {
	int howMuch = 30;
	int oneRow = height % howMuch == 0 ? height / howMuch : height / howMuch + 1;
	int sizeNewA = oneRow * width;
	oneRow = oneRow;

	int* newA = new int[sizeNewA];
	std::fill(newA, newA + sizeNewA, 0);

	for (int i = 0; i < width; i++) {
		for (int j = 0; j < height; j++) {
			int value = a[i * height + j] == 255 ? 1 : 0;

			int pos1 = i * oneRow + j / howMuch;
			int pos2 = value << (j % howMuch);
			newA[pos1] += pos2;
		}
	}

	return newA;
}

unsigned char* decompressImage(const int* a, int width, int height) {
	int howMuch = 30;
	int oneRow = height % howMuch == 0 ? height / howMuch : height / howMuch + 1;
	int sizeNewA = width * height;

	unsigned char* newA = new unsigned char[sizeNewA];
	std::fill(newA, newA + sizeNewA, 255);

	for (int i = 0; i < width; i++) {
		for (int j = 0; j < height; j++) {
			int pos1 = i * oneRow + j / howMuch;
			int pos2 = 1 << (j % howMuch);

			int value = (a[pos1] & pos2) >= 1 ? 255 : 0;

			newA[i * height + j] = (int)value;
		}
	}

	return newA;
}

char* readKernelSource(const char* filename){
	char* kernelSource = nullptr;
	long length;
	FILE* f = fopen(filename, "r");
	if (f)
	{
		fseek(f, 0, SEEK_END);
		length = ftell(f);
		fseek(f, 0, SEEK_SET);
		kernelSource = (char*)calloc(length, sizeof(char));
		if (kernelSource)
			fread(kernelSource, 1, length, f);
		fclose(f);
	}
	return kernelSource;
}

void readImage(const char* filename, unsigned char*& array, int& width, int& height){
	FILE* fp = fopen(filename, "rb"); /* b - binary mode */
	if (!fscanf(fp, "P5\n%d %d\n255\n", &width, &height)) {
		throw "error";
	}
	unsigned char* image = new unsigned char[(size_t)width * height];
	fread(image, sizeof(unsigned char), (size_t)width * (size_t)height, fp);
	fclose(fp);
	array = image;
}

void writeImage(const char* filename, const unsigned char* array, const int width, const int height){
	FILE* fp = fopen(filename, "wb"); /* b - binary mode */
	fprintf(fp, "P5\n%d %d\n255\n", width, height);
	fwrite(array, sizeof(unsigned char), (size_t)width * (size_t)height, fp);
	fclose(fp);
}

/*=====================================================*/
/*=====================================================*/
/*===================== GAME OF LIFE ==================*/
/*=====================================================*/
/*=====================================================*/

void GameOfLife(int n)
{
	for (int i = 0; i < n; i++) {
		cl_mem oldMatrix;
		cl_mem newMatrix;

		cl_platform_id idPlatform;
		cl_device_id idDevice;
		cl_context context;
		cl_command_queue queue;
		cl_program program;
		cl_kernel kernel;

		size_t globalsize[2], localsize[2];
		cl_int err;

		err = clGetPlatformIDs(1, &idPlatform, NULL);
		err = clGetDeviceIDs(idPlatform, CL_DEVICE_TYPE_GPU, 1, &idDevice, NULL);
		context = clCreateContext(0, 1, &idDevice, NULL, NULL, &err);
		queue = clCreateCommandQueue(context, idDevice, 0, &err);
		char* kernelSrc = readKernelSource("GameOfLife.cl");

		int maks = getMaxWorkGroupSize(idDevice);

		globalsize[0] = globalsize[1] = maks;

		for (int i = 8; i >= 1; i--) {
			if (maks % i == 0) {
				localsize[0] = localsize[1] = i;
				break;
			}
		}

		program = clCreateProgramWithSource(context, 1, (const char**)&kernelSrc, NULL, &err);
		err = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);

		if (err) {
			size_t logSize;
			clGetProgramBuildInfo(program, idDevice, CL_PROGRAM_BUILD_LOG, 0, NULL, &logSize);
			char* log = new char[logSize];
			clGetProgramBuildInfo(program, idDevice, CL_PROGRAM_BUILD_LOG, logSize, log, &logSize);
			std::cout << log << std::endl;

			delete[]log;
		}

		kernel = clCreateKernel(program, "GameOfLife", &err);
		size_t workGroupSize = 0;
		clGetKernelWorkGroupInfo(kernel, idDevice, CL_KERNEL_WORK_GROUP_SIZE, sizeof(size_t), &workGroupSize, nullptr);

		oldMatrix = clCreateBuffer(context, CL_MEM_READ_WRITE, imageSize * sizeof(int), NULL, NULL);
		newMatrix = clCreateBuffer(context, CL_MEM_READ_WRITE, imageSize * sizeof(int), NULL, NULL);

		err = clEnqueueWriteBuffer(queue, oldMatrix, CL_TRUE, 0, imageSize * sizeof(int), mat, 0, NULL, NULL);
		err |= clEnqueueWriteBuffer(queue, newMatrix, CL_TRUE, 0, imageSize * sizeof(int), mat, 0, NULL, NULL);

		err = clSetKernelArg(kernel, 0, sizeof(cl_mem), &oldMatrix);
		err |= clSetKernelArg(kernel, 1, sizeof(cl_mem), &newMatrix);
		err |= clSetKernelArg(kernel, 2, sizeof(int), &width);
		err |= clSetKernelArg(kernel, 3, sizeof(int), &height);

		err = clEnqueueNDRangeKernel(queue, kernel, 2, NULL, globalsize, localsize, 0, NULL, NULL);

		clFinish(queue);
		clEnqueueReadBuffer(queue, newMatrix, CL_TRUE, 0, imageSize * sizeof(int), mat, 0, NULL, NULL);
		clFinish(queue);

		clReleaseMemObject(oldMatrix);
		clReleaseMemObject(newMatrix);
		clReleaseProgram(program);
		clReleaseKernel(kernel);
		clReleaseCommandQueue(queue);
		clReleaseContext(context);

		free(kernelSrc);

		const std::string outFile = std::string("pictures/image") + std::to_string(i + 1) + std::string(".pgm");
		unsigned char* mat2 = decompressImage(mat, width, height);
		writeImage(outFile.c_str(), mat2, width, height);
		delete[]mat2;
	}
}

/*=====================================================*/
/*=====================================================*/
/*PRINTING ALL INFORMATION: ABOUT DEVICES AND PLATFORMS*/
/*=====================================================*/
/*=====================================================*/

void printAllInformation() {
	cout << "Informacije sistema: " << endl << endl;

	cl_uint num;
	clGetPlatformIDs(0, NULL, &num);
	cout << "Broj Platformi koji podrzavaju openCL: " << num << endl << endl;

	cl_platform_id* arr_p = new cl_platform_id[num];
	clGetPlatformIDs(num, arr_p, NULL);

	for (int i = 0; i < (int)num; i++) {
		cout << "-- Platform [" << i << "]: " << endl;

		char buffer[1024];
		clGetPlatformInfo(arr_p[i], CL_PLATFORM_NAME, sizeof(buffer), &buffer, NULL);
		cout << "	Platform Name: " << buffer << endl;

		clGetPlatformInfo(arr_p[i], CL_PLATFORM_VENDOR, sizeof(buffer), &buffer, NULL);
		cout << "	Platform Vendor: " << buffer << endl;

		clGetPlatformInfo(arr_p[i], CL_PLATFORM_VERSION, sizeof(buffer), &buffer, NULL);
		cout << "	OpenCL Version: " << buffer << endl;


		cl_uint num_d;
		clGetDeviceIDs(arr_p[i], CL_DEVICE_TYPE_ALL, 0, NULL, &num_d);

		cl_device_id* arr_d = new cl_device_id[num_d];
		clGetDeviceIDs(arr_p[i], CL_DEVICE_TYPE_ALL, num_d, arr_d, NULL);

		for (int j = 0; j < (int)num_d; j++) {
			cl_ulong aa;
			size_t bb;
			cl_uint cc;
			cout << "---- Device [" << j << "]: " << endl;
			clGetDeviceInfo(arr_d[i], CL_DEVICE_NAME, sizeof(buffer), &buffer, NULL);
			cout << "	Name: " << buffer << endl;

			clGetDeviceInfo(arr_d[i], CL_DEVICE_VERSION, sizeof(buffer), &buffer, NULL);
			cout << "	Device version: " << buffer << endl;

			clGetDeviceInfo(arr_d[i], CL_DRIVER_VERSION, sizeof(buffer), &buffer, NULL);
			cout << "	Driver version: " << buffer << endl;

			clGetDeviceInfo(arr_d[i], CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(aa), &aa, NULL);
			cout << "	Max compute units: " << aa << endl;

			clGetDeviceInfo(arr_d[i], CL_DEVICE_GLOBAL_MEM_SIZE, sizeof(aa), &aa, NULL);
			cout << "	Global memory size: " << aa / 1024 / 1024 << endl;

			clGetDeviceInfo(arr_d[i], CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(bb), &bb, NULL);
			cout << "	Max work group size: " << bb << endl;

			clGetDeviceInfo(arr_d[i], CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS, sizeof(cc), &cc, NULL);
			cout << "	Max work items dimensions: " << cc << endl;

			size_t buf[3];
			clGetDeviceInfo(arr_d[i], CL_DEVICE_MAX_WORK_ITEM_SIZES, sizeof(buf), &buf, NULL);
			for (int k = 0; k < (int)cc; k++) {
				cout << "	Dimension[" << k << "]: " << buf[k] << endl;
			}

			cout << endl;
		}
	}

	delete[]arr_p;
}

int getMaxWorkGroupSize(cl_device_id idDevice) {
	size_t value;
	clGetDeviceInfo(idDevice, CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(value), &value, NULL);
	return (int)value;
}

int getMaxWorkGroupSize() {
	cl_device_id idDevice;
	cl_platform_id idPlatform;
	clGetPlatformIDs(1, &idPlatform, NULL);
	clGetDeviceIDs(idPlatform, CL_DEVICE_TYPE_GPU, 1, &idDevice, NULL);
	size_t value;
	clGetDeviceInfo(idDevice, CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(value), &value, NULL);
	return (int)value;
}

/*=====================================================*/
/*=====================================================*/
/*================= COPY SUBSEGMENT ===================*/
/*=====================================================*/
/*=====================================================*/

void copySubSegmentOfBoard(int x1, int y1, int x2, int y2) {
	int x = (x2 - x1);
	int y = (y2 - y1);

	int* mat1 = nullptr;


	int oneRow1 = oneRow;
	int oneRow2 = (y % howMuch == 0 ? y / howMuch : y / howMuch + 1);
	int imageSize1 = imageSize;
	int imageSize2 = x * oneRow2;

	mat1 = new int[imageSize2];
	for (int i = 0; i < imageSize2; i++) {
		mat1[i] = 0;
	}

	cl_mem matrix1;
	cl_mem matrix2;

	cl_platform_id idPlatform;
	cl_device_id idDevice;
	cl_context context;
	cl_command_queue queue;
	cl_program program;
	cl_kernel kernel;

	size_t globalsize[2], localsize[2];
	cl_int err;

	globalsize[0] = globalsize[1] = std::max(width, height);

	for (int i = 8; i >= 1; i--) {
		if (std::max(width, height) % i == 0) {
			localsize[0] = localsize[1] = i;
			break;
		}
	}

	err = clGetPlatformIDs(1, &idPlatform, NULL);
	err = clGetDeviceIDs(idPlatform, CL_DEVICE_TYPE_GPU, 1, &idDevice, NULL);
	context = clCreateContext(0, 1, &idDevice, NULL, NULL, &err);
	queue = clCreateCommandQueue(context, idDevice, 0, &err);
	char* kernelSrc = readKernelSource("copySubSegment.cl");

	program = clCreateProgramWithSource(context, 1, (const char**)&kernelSrc, NULL, &err);
	err = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);

	if (err) {
		size_t logSize;
		clGetProgramBuildInfo(program, idDevice, CL_PROGRAM_BUILD_LOG, 0, NULL, &logSize);
		char* log = new char[logSize];
		clGetProgramBuildInfo(program, idDevice, CL_PROGRAM_BUILD_LOG, logSize, log, &logSize);
		std::cout << log << std::endl;

		delete[]log;
	}

	kernel = clCreateKernel(program, "copySubSegment", &err);
	size_t workGroupSize = 0;
	clGetKernelWorkGroupInfo(kernel, idDevice, CL_KERNEL_WORK_GROUP_SIZE, sizeof(size_t), &workGroupSize, nullptr);

	matrix1 = clCreateBuffer(context, CL_MEM_READ_WRITE, imageSize1 * sizeof(int), NULL, NULL);
	matrix2 = clCreateBuffer(context, CL_MEM_READ_WRITE, imageSize2 * sizeof(int), NULL, NULL);

	err = clEnqueueWriteBuffer(queue, matrix1, CL_TRUE, 0, imageSize1 * sizeof(int), mat, 0, NULL, NULL);
	err |= clEnqueueWriteBuffer(queue, matrix2, CL_TRUE, 0, imageSize2 * sizeof(int), mat1, 0, NULL, NULL);

	err = clSetKernelArg(kernel, 0, sizeof(cl_mem), &matrix1);
	err |= clSetKernelArg(kernel, 1, sizeof(cl_mem), &matrix2);
	err |= clSetKernelArg(kernel, 2, sizeof(int), &height);
	err |= clSetKernelArg(kernel, 3, sizeof(int), &x1);
	err |= clSetKernelArg(kernel, 4, sizeof(int), &y1);
	err |= clSetKernelArg(kernel, 5, sizeof(int), &x2);
	err |= clSetKernelArg(kernel, 6, sizeof(int), &y2);
	err |= clSetKernelArg(kernel, 7, sizeof(int), &y);

	err = clEnqueueNDRangeKernel(queue, kernel, 2, NULL, globalsize, localsize, 0, NULL, NULL);

	clFinish(queue);
	clEnqueueReadBuffer(queue, matrix2, CL_TRUE, 0, imageSize2 * sizeof(int), mat1, 0, NULL, NULL);
	clFinish(queue);

	clReleaseMemObject(matrix1);
	clReleaseMemObject(matrix2);
	clReleaseProgram(program);
	clReleaseKernel(kernel);
	clReleaseCommandQueue(queue);
	clReleaseContext(context);

	free(kernelSrc);

	unsigned char* mat2 = decompressImage(mat1, x, y);
	writeImage("copySubSegment/result.pgm", mat2, x, y);
	delete[]mat2;
	delete[]mat1;
}

/*=====================================================*/
/*=====================================================*/
/*==================== INIT COPY ======================*/
/*=====================================================*/
/*=====================================================*/

void initCopyStates(const std::string path, int x1, int y1, int x2, int y2) {
	int width2 = -1;
	int height2 = -1;
	int* mat2 = nullptr;
	unsigned char* m2 = nullptr;
	readImage(path.c_str(), m2, width2, height2);
	mat2 = compressImage(m2, width2, height2);

	int oneRow1 = oneRow;
	int oneRow2 = (height2 % howMuch == 0 ? height2 / howMuch : height2 / howMuch + 1);
	int imageSize1 = imageSize;
	int imageSize2 = width2 * oneRow2;

	cl_mem matrix1;
	cl_mem matrix2;

	cl_platform_id idPlatform;
	cl_device_id idDevice;
	cl_context context;
	cl_command_queue queue;
	cl_program program;
	cl_kernel kernel;

	size_t globalsize[2], localsize[2];
	cl_int err;

	for (int i = 8; i >= 1; i--) {
		if (std::max(width, height) % i == 0) {
			localsize[0] = localsize[1] = i;
			break;
		}
	}

	globalsize[0] = globalsize[1] = std::max(width, height);

	err = clGetPlatformIDs(1, &idPlatform, NULL);
	err = clGetDeviceIDs(idPlatform, CL_DEVICE_TYPE_GPU, 1, &idDevice, NULL);
	context = clCreateContext(0, 1, &idDevice, NULL, NULL, &err);
	queue = clCreateCommandQueue(context, idDevice, 0, &err);
	char* kernelSrc = readKernelSource("init02.cl");

	program = clCreateProgramWithSource(context, 1, (const char**)&kernelSrc, NULL, &err);
	err = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);

	if (err) {
		size_t logSize;
		clGetProgramBuildInfo(program, idDevice, CL_PROGRAM_BUILD_LOG, 0, NULL, &logSize);
		char* log = new char[logSize];
		clGetProgramBuildInfo(program, idDevice, CL_PROGRAM_BUILD_LOG, logSize, log, &logSize);
		std::cout << log << std::endl;

		delete[]log;
	}

	kernel = clCreateKernel(program, "init02", &err);
	size_t workGroupSize = 0;
	clGetKernelWorkGroupInfo(kernel, idDevice, CL_KERNEL_WORK_GROUP_SIZE, sizeof(size_t), &workGroupSize, nullptr);

	matrix1 = clCreateBuffer(context, CL_MEM_READ_WRITE, imageSize1 * sizeof(int), NULL, NULL);
	matrix2 = clCreateBuffer(context, CL_MEM_READ_WRITE, imageSize2 * sizeof(int), NULL, NULL);

	err = clEnqueueWriteBuffer(queue, matrix1, CL_TRUE, 0, imageSize1 * sizeof(int), mat, 0, NULL, NULL);
	err |= clEnqueueWriteBuffer(queue, matrix2, CL_TRUE, 0, imageSize2 * sizeof(int), mat2, 0, NULL, NULL);

	err = clSetKernelArg(kernel, 0, sizeof(cl_mem), &matrix1);
	err |= clSetKernelArg(kernel, 1, sizeof(cl_mem), &matrix2);
	err |= clSetKernelArg(kernel, 2, sizeof(int), &height);
	err |= clSetKernelArg(kernel, 3, sizeof(int), &x1);
	err |= clSetKernelArg(kernel, 4, sizeof(int), &y1);
	err |= clSetKernelArg(kernel, 5, sizeof(int), &x2);
	err |= clSetKernelArg(kernel, 6, sizeof(int), &y2);
	err |= clSetKernelArg(kernel, 7, sizeof(int), &height2);

	err = clEnqueueNDRangeKernel(queue, kernel, 2, NULL, globalsize, localsize, 0, NULL, NULL);

	clFinish(queue);
	clEnqueueReadBuffer(queue, matrix1, CL_TRUE, 0, imageSize1 * sizeof(int), mat, 0, NULL, NULL);
	clFinish(queue);

	clReleaseMemObject(matrix1);
	clReleaseMemObject(matrix2);
	clReleaseProgram(program);
	clReleaseKernel(kernel);
	clReleaseCommandQueue(queue);
	clReleaseContext(context);

	free(kernelSrc);

	unsigned char* mat3 = decompressImage(mat, width, height);
	writeImage("initCopyStates/result.pgm", mat3, width, height);
	delete[]mat3;

	delete[]mat2;
	delete[]m2;
}

/*=====================================================*/
/*=====================================================*/
/*=================== INIT STATES =====================*/
/*=====================================================*/
/*=====================================================*/

void initMyStates(vector <pair<int, int> > arr) {
	int* pX = new int[arr.size()];
	int* pY = new int[arr.size()];
	for (int i = 0; i < arr.size(); i++) {
		if (arr[i].first >= width || arr[i].first < 0 || arr[i].second < 0 || arr[i].second >= height) {
			continue;
		}

		pX[i] = arr[i].first;
		pY[i] = arr[i].second;
	}

	int div = -1;
	for (int i = 2; i < arr.size(); i++) {
		if (arr.size() % i == 0) {
			div = i;
			break;
		}
	}

	int* mat1 = nullptr;

	int oneRow = (height % howMuch == 0 ? height / howMuch : height / howMuch + 1);
	int imageSize = width * oneRow;
	mat1 = new int[imageSize];
	std::fill(mat1, mat1 + imageSize, INT_MAX);

	cl_mem matrix;
	cl_mem PX;
	cl_mem PY;

	cl_platform_id idPlatform;
	cl_device_id idDevice;
	cl_context context;
	cl_command_queue queue;
	cl_program program;
	cl_kernel kernel;

	size_t globalsize[1], localsize[1];
	cl_int err;

	localsize[0] = div == -1 ? 1 : div;
	globalsize[0] = arr.size();

	err = clGetPlatformIDs(1, &idPlatform, NULL);
	err = clGetDeviceIDs(idPlatform, CL_DEVICE_TYPE_GPU, 1, &idDevice, NULL);
	context = clCreateContext(0, 1, &idDevice, NULL, NULL, &err);
	queue = clCreateCommandQueue(context, idDevice, 0, &err);
	char* kernelSrc = readKernelSource("init01.cl");

	program = clCreateProgramWithSource(context, 1, (const char**)&kernelSrc, NULL, &err);
	err = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);

	if (err) {
		size_t logSize;
		clGetProgramBuildInfo(program, idDevice, CL_PROGRAM_BUILD_LOG, 0, NULL, &logSize);
		char* log = new char[logSize];
		clGetProgramBuildInfo(program, idDevice, CL_PROGRAM_BUILD_LOG, logSize, log, &logSize);
		std::cout << log << std::endl;

		delete[]log;
	}

	kernel = clCreateKernel(program, "init01", &err);
	size_t workGroupSize = 0;
	clGetKernelWorkGroupInfo(kernel, idDevice, CL_KERNEL_WORK_GROUP_SIZE, sizeof(size_t), &workGroupSize, nullptr);

	matrix = clCreateBuffer(context, CL_MEM_READ_WRITE, imageSize * sizeof(int), NULL, NULL);
	PX = clCreateBuffer(context, CL_MEM_READ_WRITE, arr.size() * sizeof(int), NULL, NULL);
	PY = clCreateBuffer(context, CL_MEM_READ_WRITE, arr.size() * sizeof(int), NULL, NULL);

	err = clEnqueueWriteBuffer(queue, matrix, CL_TRUE, 0, imageSize * sizeof(int), mat1, 0, NULL, NULL);
	err |= clEnqueueWriteBuffer(queue, PX, CL_TRUE, 0, arr.size() * sizeof(int), pX, 0, NULL, NULL);
	err |= clEnqueueWriteBuffer(queue, PY, CL_TRUE, 0, arr.size() * sizeof(int), pY, 0, NULL, NULL);

	err = clSetKernelArg(kernel, 0, sizeof(cl_mem), &matrix);
	err |= clSetKernelArg(kernel, 1, sizeof(cl_mem), &PX);
	err |= clSetKernelArg(kernel, 2, sizeof(cl_mem), &PY);
	err |= clSetKernelArg(kernel, 3, sizeof(int), &height);

	err = clEnqueueNDRangeKernel(queue, kernel, 1, NULL, globalsize, localsize, 0, NULL, NULL);

	clFinish(queue);
	clEnqueueReadBuffer(queue, matrix, CL_TRUE, 0, imageSize * sizeof(int), mat1, 0, NULL, NULL);
	clFinish(queue);

	clReleaseMemObject(matrix);
	clReleaseMemObject(PX);
	clReleaseMemObject(PY);
	clReleaseProgram(program);
	clReleaseKernel(kernel);
	clReleaseCommandQueue(queue);
	clReleaseContext(context);

	free(kernelSrc);

	unsigned char *mat2 = decompressImage(mat1, width, height);
	writeImage("initMyStates/result.pgm", mat2, width, height);
	delete[]mat2;

	delete[]mat1;
	delete[]pX;
	delete[]pY;
}

/*=====================================================*/
/*=====================================================*/
/*========== BIG PLAYGROUND - GAME OF LIFE ============*/
/*=====================================================*/
/*=====================================================*/

void GameOfLife1(int n) {
	int maks = getMaxWorkGroupSize();
	int maksLocal = 1;

	for (int i = 8; i >= 1; i--) {
		if (maks % i == 0) {
			maksLocal = i;
			break;
		}
	}

	int howR = width % maks == 0 ? width / maks : width / maks + 1;
	int howC = height % maks == 0 ? height / maks : height / maks + 1;
	int* matBuffer1 = new int[imageSize];
	int* matBuffer2 = new int[imageSize];

	for (int i = 0; i < imageSize; i++) {
		matBuffer1[i] = matBuffer2[i] = 0;
	}

	for (int i = 0; i < n; i++) {
		for (int i1 = 0; i1 < howR; i1++) {
			for (int j1 = 0; j1 < howC; j1++) {
				int xStart = i1 * maks;
				int yStart = j1 * maks;

				cl_mem oldMatrix;
				cl_mem newMatrix;

				cl_platform_id idPlatform;
				cl_device_id idDevice;
				cl_context context;
				cl_command_queue queue;
				cl_program program;
				cl_kernel kernel;

				size_t globalsize[2], localsize[2];
				cl_int err;

				localsize[0] = localsize[1] = maksLocal;
				globalsize[0] = globalsize[1] = maks;

				err = clGetPlatformIDs(1, &idPlatform, NULL);
				err = clGetDeviceIDs(idPlatform, CL_DEVICE_TYPE_GPU, 1, &idDevice, NULL);
				context = clCreateContext(0, 1, &idDevice, NULL, NULL, &err);
				queue = clCreateCommandQueue(context, idDevice, 0, &err);
				char* kernelSrc = readKernelSource("GameOfLife1.cl");

				program = clCreateProgramWithSource(context, 1, (const char**)&kernelSrc, NULL, &err);
				err = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
	
				if (err) {
					size_t logSize;
					clGetProgramBuildInfo(program, idDevice, CL_PROGRAM_BUILD_LOG, 0, NULL, &logSize);
					char* log = new char[logSize];
					clGetProgramBuildInfo(program, idDevice, CL_PROGRAM_BUILD_LOG, logSize, log, &logSize);
					std::cout << log << std::endl;

					delete[]log;
				}

				kernel = clCreateKernel(program, "GameOfLife", &err);
				size_t workGroupSize = 0;
				clGetKernelWorkGroupInfo(kernel, idDevice, CL_KERNEL_WORK_GROUP_SIZE, sizeof(size_t), &workGroupSize, nullptr);

				oldMatrix = clCreateBuffer(context, CL_MEM_READ_WRITE, imageSize * sizeof(int), NULL, NULL);
				newMatrix = clCreateBuffer(context, CL_MEM_READ_WRITE, imageSize * sizeof(int), NULL, NULL);

				err = clEnqueueWriteBuffer(queue, oldMatrix, CL_TRUE, 0, imageSize * sizeof(int), mat, 0, NULL, NULL);
				err |= clEnqueueWriteBuffer(queue, newMatrix, CL_TRUE, 0, imageSize * sizeof(int), mat, 0, NULL, NULL);

				err = clSetKernelArg(kernel, 0, sizeof(cl_mem), &oldMatrix);
				err |= clSetKernelArg(kernel, 1, sizeof(cl_mem), &newMatrix);
				err |= clSetKernelArg(kernel, 2, sizeof(int), &xStart);
				err |= clSetKernelArg(kernel, 3, sizeof(int), &yStart);
				err |= clSetKernelArg(kernel, 4, sizeof(int), &width);
				err |= clSetKernelArg(kernel, 5, sizeof(int), &height);

				err = clEnqueueNDRangeKernel(queue, kernel, 2, NULL, globalsize, localsize, 0, NULL, NULL);

				clFinish(queue);
				clEnqueueReadBuffer(queue, newMatrix, CL_TRUE, 0, imageSize * sizeof(int), matBuffer1, 0, NULL, NULL);
				clFinish(queue);

				clReleaseMemObject(oldMatrix);
				clReleaseMemObject(newMatrix);
				clReleaseProgram(program);
				clReleaseKernel(kernel);
				clReleaseCommandQueue(queue);
				clReleaseContext(context);

				free(kernelSrc);
			
				for (int i2 = i1 * maks; i2 < i1 * maks + maks && i2 < width; i2++) {
					for (int j2 = j1 * maks; j2 < j1 * maks + maks && j2 < height; j2++) {
						int pos1 = i2 * oneRow + j2 / howMuch;
						int pos2 = 1 << (j2 % howMuch);

						int value11 = matBuffer1[pos1] & pos2;

						if (value11 > 0) {
							int value22 = matBuffer2[pos1] & pos2;

							if (value22 == 0) {
								matBuffer2[pos1] -= pos2;
							}
						}
						else {
							int value22 = matBuffer2[pos1] & pos2;

							if (value22 > 0) {
								matBuffer2[pos1] -= pos2;
							}
						}
					}
				}
			}
		}

		for (int i2 = 0; i2 < imageSize; i2++) {
			mat[i2] = (int)matBuffer2[i2];
		}

		const std::string outFile = std::string("GameOfLife1/image") + std::to_string(i + 1) + std::string(".pgm");
		unsigned char* matPrint = decompressImage(mat, width, height);
		writeImage(outFile.c_str(), matPrint, width, height);
		delete[]matPrint;
	}

	delete[]matBuffer1;
	delete[]matBuffer2;
}

/*=====================================================*/
/*=====================================================*/
/*Ime i prezime: Aleksandar Tulic =====================*/
/*Indeks	   : 1179/18 ==============================*/
/*=====================================================*/
/*=====================================================*/