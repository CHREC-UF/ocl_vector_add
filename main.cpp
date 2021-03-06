#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string>
#include <sys/types.h>
#include <CL/opencl.h>

#define MAX_PLATFORMS 10
#define MAX_DEVICES 16

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

  cl_platform_id platform[MAX_PLATFORMS];
  cl_device_id device[MAX_DEVICES];
  cl_context context[MAX_DEVICES];
  cl_command_queue queue[MAX_DEVICES];
  cl_program program[MAX_DEVICES];
  cl_kernel kernel[MAX_DEVICES];

  char cl_platform_vendor[MAX_PLATFORMS][1001];
  char cl_platform_name[MAX_PLATFORMS][1001];

  char cl_device_name[1001];

  cl_mem input_a[MAX_DEVICES];
  cl_mem input_b[MAX_DEVICES];
  cl_mem output_c[MAX_DEVICES];

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

  if (num_platforms > MAX_PLATFORMS) num_platforms = MAX_PLATFORMS;

  printf("Found %i OpenCL platforms\n\n\n\n", num_platforms);

  err = clGetPlatformIDs(MAX_PLATFORMS, platform, NULL);
  if (err != CL_SUCCESS)
  {
    printf("ERROR: Failed to find OpenCL platform\n");
    return -1;
  }

  for(unsigned i=0; i<num_platforms; i++)
  {
    err = clGetPlatformInfo(platform[i], CL_PLATFORM_VENDOR, 1000, 
        (void*) cl_platform_vendor[i], NULL);

    err |= clGetPlatformInfo(platform[i], CL_PLATFORM_NAME, 1000,
        (void *) cl_platform_name[i], NULL);
    if (err != CL_SUCCESS)
    {
      printf("ERROR: Failed to get OpenCL platform info\n");
      return -1;
    }

    printf("    OpenCL platform %i: %s OpenCL name: %s\n", i, cl_platform_vendor[i],
        cl_platform_name[i]);
  }

  printf("\n\nChoose OpenCL platform: ");
  int number = 0;

  scanf("%d", &number);

  printf("    Using OpenCL platform: %s\n\n", cl_platform_name[number]);
 
  std::string myPlatform(cl_platform_name[number]);
  std::string input_file("vector_add");

  if (argc == 2)
  {
    input_file.assign(argv[1]);
  } else
  {
    if (myPlatform.compare("Xilinx") == 0)
    {
      input_file.append(".xclbin");
    } else if (myPlatform.compare("Intel(R) OpenCL") == 0)
    {
      input_file.append(".cl");
    } else if (myPlatform.compare("Altera SDK for OpenCL") == 0)
    {
      input_file.append(".aocx");
    }
  }    

  if (access(input_file.c_str(), F_OK) != 0)
  {
    printf("ERROR: OpenCL file %s does not exist\n", input_file.c_str());
    return -1;
  }


  const int device_flag = CL_DEVICE_TYPE_DEFAULT;

  cl_uint num_devices;

  err = clGetDeviceIDs(platform[number], device_flag, 0, NULL, &num_devices);
  if (err != CL_SUCCESS)
  {
    printf("ERROR: Failed finding OpenCL devices\n");
    return -1;
  }

  if (num_devices > MAX_DEVICES) num_devices = MAX_DEVICES;
  printf("Found %d OpenCL devices\n", num_devices);

  err = clGetDeviceIDs(platform[number], device_flag, num_devices, device, NULL);
  if (err != CL_SUCCESS)
  {
    printf("ERROR: Failed finding OpenCL device\n");
    return -1;
  }

  err = clGetDeviceInfo(device[0], CL_DEVICE_NAME, 1000, cl_device_name, NULL);
  if (err != CL_SUCCESS)
  {
    printf("ERROR: Failed getting device name\n");
    return -1;
  }

  printf("\nUsing OpenCL device: %s\n", cl_device_name);

  bool pass = true;
  int i = 0;
  for (i=0; i<num_devices; i++)
  {

    context[i] = clCreateContext(0, 1, &device[i], NULL, NULL, &err);
    if (err != CL_SUCCESS)
    {
      printf("ERROR: Failed creating context\n");
      return -1;
    }

    queue[i] = clCreateCommandQueue(context[i], device[i], 0, &err);
    if (err != CL_SUCCESS)
    {
      printf("ERROR: Failed to create command queue\n");
      return -1;
    }

    unsigned char *binary;
    const char *bitfile = input_file.c_str();
    size_t binary_size = load_file_to_memory(bitfile, (char **) &binary);
    if(binary_size < 0)
    {
      printf("ERROR: Failed to load binary\n");
      return -1;
    }

    if(myPlatform.compare("Intel(R) OpenCL") == 0)
    {
      program[i] = clCreateProgramWithSource(context[i], 1, 
          (const char **) &binary, &binary_size, &err);
    } else{
      cl_int status;
      program[i] = clCreateProgramWithBinary(context[i], 1, &device[i], &binary_size,
          (const unsigned char **) &binary, &status, &err);
    }
    if (err != CL_SUCCESS)
    {
      printf("ERROR: Failed to create OpenCL program\n");
    }
    err = clBuildProgram(program[i], 0, NULL, NULL, NULL, NULL);
    if (err != CL_SUCCESS)
    {
      printf("ERROR: Failed to build OpenCL program\n");
      return -1;
    } 


    kernel[i] = clCreateKernel(program[i], "vector_add", &err);
    if (err != CL_SUCCESS)
    {
      printf("ERROR: Failed to create OpenCL kernel\n");
      return -1;
    }

    input_a[i] = clCreateBuffer(context[i], CL_MEM_READ_ONLY, sizeof(int) * DATA_SIZE,
        NULL, NULL);
    input_b[i] = clCreateBuffer(context[i], CL_MEM_READ_ONLY, sizeof(int) * DATA_SIZE,
        NULL, NULL);
    output_c[i] = clCreateBuffer(context[i], CL_MEM_WRITE_ONLY, sizeof(int) 
        * DATA_SIZE, NULL, NULL);
    if(!input_a[i] || !input_b[i] || !output_c[i])
    {
      printf("ERROR: Failed to allocate OpenCL buffers\n");
      return -1;
    }

    err = clEnqueueWriteBuffer(queue[i], input_a[i], CL_TRUE, 0, sizeof(int)
        * DATA_SIZE, a, 0, NULL, NULL);
    err |= clEnqueueWriteBuffer(queue[i], input_b[i], CL_TRUE, 0, sizeof(int)
        * DATA_SIZE, b, 0, NULL, NULL);
    if (err != CL_SUCCESS)
    {
      printf("ERROR: Failed to send input data\n");
      return -1;
    }

    err  = clSetKernelArg(kernel[i], 0, sizeof(cl_mem), &input_a[i]);
    err |= clSetKernelArg(kernel[i], 1, sizeof(cl_mem), &input_b[i]);
    err |= clSetKernelArg(kernel[i], 2, sizeof(cl_mem), &output_c[i]);
    if (err != CL_SUCCESS)
    {
      printf("ERROR: Failed to set OpenCL kernel arguments\n");
      return -1;
    }

    global = DATA_SIZE;
    local = 16;

    err = clEnqueueNDRangeKernel(queue[i], kernel[i], 1, NULL, &global, &local, 0, 
        NULL, NULL);
    if (err != CL_SUCCESS)
    {
      printf("ERROR: Failed to run OpenCL kernel\n");
      return -1;
    }

    err = clFinish(queue[i]);
    if (err != CL_SUCCESS)
    {
      printf("ERROR: Command queue failed to complete\n");
      return -1;
    }

    err = clEnqueueReadBuffer(queue[i], output_c[i], CL_TRUE, 0, sizeof(int) 
        * DATA_SIZE, c, 0, NULL, NULL);
    if (err != CL_SUCCESS)
    {
      printf("ERROR: Failed to read OpenCL buffer\n");
      return -1;
    }


    for(int i=0; i<DATA_SIZE; i++)
    {
      if (c[i] != DATA_SIZE)
      {
        pass = false;
        break;
      }
    }
  }

  printf("OpenCL test %s\n",pass?"PASS":"FAIL");

  for (i=0; i<num_devices; i++)
  {
    clReleaseMemObject(input_a[i]);
    clReleaseMemObject(input_b[i]);
    clReleaseMemObject(output_c[i]);
    clReleaseKernel(kernel[i]);
    clReleaseProgram(program[i]);
    clReleaseCommandQueue(queue[i]);
    clReleaseContext(context[i]);
  }

  return 0;

}

