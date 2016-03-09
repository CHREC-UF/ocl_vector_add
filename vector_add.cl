__kernel void vector_add(
    __global int *a,
    __global int *b,
    __global int *c)
{
#if AOC_EMULATE
  if (get_global_id(0) == 1) printf("Hello from work-item 0\n");
#endif
  
  int index = get_global_id(0);
  c[index] = a[index] + b[index];
}

