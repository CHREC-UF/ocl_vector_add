__kernel void vector_add(
    __global int *a,
    __global int *b,
    __global int *c)
{
  int index = get_global_id(0);
  c[index] = a[index] + b[index];
}

