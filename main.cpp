#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <CL/opencl.h>

int load_file_to_memory(const char *filename, char **result)
{
  size_t size = 0;
  FILE *file = fopen(filename, "rb");
  if (file == NULL)
  {
    *result = NULL;
    return -1;
  }
  fseek(file, 0, SEEK_END);
  size = ftell(file);
  fseek(file, 0, SEEK_SET);
  *result = (char*) malloc(size+1);
  if (size != fread(*result, sizeof(char), size, file))
  {
    free(*result);
    return -1;
  }
  fclose(file);
  (*result)[size] = 0;
  return size;
}

#define DATA_SIZE 1024

int main(int argc, char *argv[])
{
  int err;

  int a[DATA_SIZE];
  int b[DATA_SIZE];
  int c[DATA_SIZE];

  size_t global;
  size_t local;

  cl_platform_id platform;
  cl_device_id device;
  cl_context context;
  cl_command_queue queue;
  cl_program program;
  cl_kernel kernel;

  char cl_platform_vendor[1001];
  char cl_platform_name[1001];

  cl_mem input_a;
  cl_mem input_b;
  cl_mem output_c;

  if (argc != 2)
  {
    printf("%s <inputfile>\n", argv[0]);
    return -1;
  }

  for(int i=0; i<DATA_SIZE; i++)
  {
    a[i] = i;
    b[i] = DATA_SIZE - i;
    c[i] = 0;
  }

  cl_uint num_platforms = 0;
  err = clGetPlatformIDs(0, NULL, &num_platforms);
  if (err != CL_SUCCESS)
  {
    printf("ERROR: Failed to find OpenCL platforms\n");
    return -1;
  }

  printf("Found %i OpenCL platforms\n", num_platforms);

  for(unsigned i=0; i<num_platforms; i++)
  {

    err = clGetPlatformIDs(1, &platform, NULL);
    if (err != CL_SUCCESS)
    {
      printf("ERROR: Failed to find OpenCL platform\n");
      return -1;
    }

    err = clGetPlatformInfo(platform, CL_PLATFORM_VENDOR, 1000, 
        (void*) cl_platform_vendor, NULL);

    err |= clGetPlatformInfo(platform, CL_PLATFORM_NAME, 1000,
        (void *) cl_platform_name, NULL);
    if (err != CL_SUCCESS)
    {
      printf("ERROR: Failed to get OpenCL platform info\n");
      return -1;
    }

    printf("OpenCL platform: %s\nOpenCL name: %s\n", cl_platform_vendor,
        cl_platform_name);
  }
/* TODO
 * Change device flag
 *
 * e.g. CL_DEVICE_TYPE_CPU
 *
 */
#ifdef INTEL_CL
  const int device_flag = CL_DEVICE_TYPE_CPU;
#else  
  const int device_flag = CL_DEVICE_TYPE_ACCELERATOR;
#endif
  err = clGetDeviceIDs(platform, device_flag, 1, &device, NULL);
  if (err != CL_SUCCESS)
  {
    printf("ERROR: Failed finding OpenCL device\n");
    return -1;
  }

  context = clCreateContext(0, 1, &device, NULL, NULL, &err);
  if (err != CL_SUCCESS)
  {
    printf("ERROR: Failed creating context\n");
    return -1;
  }

  queue = clCreateCommandQueue(context, device, 0, &err);
  if (err != CL_SUCCESS)
  {
    printf("ERROR: Failed to create command queue\n");
    return -1;
  }

  unsigned char *binary;
  char *bitfile = argv[1];
  size_t binary_size = load_file_to_memory(bitfile, (char **) &binary);
  if(binary_size < 0)
  {
    printf("ERROR: Failed to load binary\n");
    return -1;
  }

/* TODO
 * Change to clCreateProgramWithSource 
 *
 * see www.khronos.org/registry/cl/sdk/1.0/docs/man/xhtml/clCreateProgramWithSource.html
 *
 */

#ifdef INTEL_CL

  program = clCreateProgramWithSource(context, 1, 
      (const char **) &binary, &binary_size, &err);

#else
  cl_int status;
  program = clCreateProgramWithBinary(context, 1, &device, &binary_size,
      (const unsigned char **) &binary, &status, &err);

#endif  
  if (err != CL_SUCCESS)
  {
    printf("ERROR: Failed to create OpenCL program\n");
  }
  err = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
  if (err != CL_SUCCESS)
  {
    printf("ERROR: Failed to build OpenCL program\n");
    return -1;
  } 

  kernel = clCreateKernel(program, "vector_add", &err);
  if (err != CL_SUCCESS)
  {
    printf("ERROR: Failed to create OpenCL kernel\n");
    return -1;
  }

  input_a = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(int) * DATA_SIZE,
      NULL, NULL);
  input_b = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(int) * DATA_SIZE,
      NULL, NULL);
  output_c = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(int) 
      * DATA_SIZE, NULL, NULL);
  if(!input_a || !input_b || !output_c)
  {
    printf("ERROR: Failed to allocate OpenCL buffers\n");
    return -1;
  }

  err = clEnqueueWriteBuffer(queue, input_a, CL_TRUE, 0, sizeof(int)
      * DATA_SIZE, a, 0, NULL, NULL);
  err |= clEnqueueWriteBuffer(queue, input_b, CL_TRUE, 0, sizeof(int)
      * DATA_SIZE, b, 0, NULL, NULL);
  if (err != CL_SUCCESS)
  {
    printf("ERROR: Failed to send input data\n");
    return -1;
  }

  err  = clSetKernelArg(kernel, 0, sizeof(cl_mem), &input_a);
  err |= clSetKernelArg(kernel, 1, sizeof(cl_mem), &input_b);
  err |= clSetKernelArg(kernel, 2, sizeof(cl_mem), &output_c);
  if (err != CL_SUCCESS)
  {
    printf("ERROR: Failed to set OpenCL kernel arguments\n");
    return -1;
  }

  global = DATA_SIZE;
  local = 16;

  err = clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &global, &local, 0, 
      NULL, NULL);
  if (err != CL_SUCCESS)
  {
    printf("ERROR: Failed to run OpenCL kernel\n");
    return -1;
  }

  err = clFinish(queue);
  if (err != CL_SUCCESS)
  {
    printf("ERROR: Command queue failed to complete\n");
    return -1;
  }

  err = clEnqueueReadBuffer(queue, output_c, CL_TRUE, 0, sizeof(int) 
      * DATA_SIZE, c, 0, NULL, NULL);
  if (err != CL_SUCCESS)
  {
    printf("ERROR: Failed to read OpenCL buffer\n");
    return -1;
  }

  bool pass = true;

  for(int i=0; i<DATA_SIZE; i++)
  {
    if (c[i] != DATA_SIZE)
    {
      pass = false;
      break;
    }
  }

  printf("OpenCL test %s\n",pass?"PASS":"FAIL");

  clReleaseMemObject(input_a);
  clReleaseMemObject(input_b);
  clReleaseMemObject(output_c);
  clReleaseKernel(kernel);
  clReleaseProgram(program);
  clReleaseCommandQueue(queue);
  clReleaseContext(context);

  return 0;

}

