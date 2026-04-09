//Copyright (c) 2022-2026 Dr. Moritz Lehmann
//https://github.com/ProjectPhysX/OpenCL-Wrapper#
// OpenCL-Wrapper.cpp : Этот файл содержит функцию "main". Здесь начинается и заканчивается выполнение программы.


#include "opencl.hpp"

int main() {



	Device device(select_device_with_most_flops()); // compile OpenCL C code for the fastest available device
	//смотрим поддержку 64 битных типов
	printf("fp32 is not supported(0) or supported(1) on this device = %d\n",device.info.is_fp32_capable);
	printf("int32 is not supported(0) or supported(1) on this device = %d\n", device.info.is_int32_capable);
	printf("fp64 is not supported(0) or supported(1) on this device = %d\n", device.info.is_fp64_capable);
	printf("int64 is not supported(0) or supported(1) on this device = %d\n", device.info.is_int64_capable);

	//альтернативный механизм
	cl_device_fp_config fpConfig; 
	cl_int status = clGetDeviceInfo(device.info.cl_device(), CL_DEVICE_DOUBLE_FP_CONFIG, sizeof(fpConfig), &fpConfig, NULL);
	if (fpConfig == 0) { 
		printf("Double precision is not supported on this device.\n"); 
		return 1;
	}

	const uint N = 1024u; // size of vectors
	Memory<double> A(device, N); // allocate memory on both host and device
	Memory<double> B(device, N);
	Memory<double> C(device, N);

	Kernel add_kernel(device, N, "add_kernel", A, B, C); // kernel that runs on the device

	for (uint n = 0u; n < N; n++) {
		A[n] = 3.0f; // initialize memory
		B[n] = 2.0f;
		C[n] = 1.0f;
	}

	print_info("Value before kernel execution: C[0] = " + to_string(C[0]));

	A.write_to_device(); // copy data from host memory to device memory
	B.write_to_device();
	add_kernel.run(); // run add_kernel on the device
	C.read_from_device(); // copy data from device memory to host memory

	print_info("Value after kernel execution: C[0] = " + to_string(C[0]));

	wait();
	return 0;
}