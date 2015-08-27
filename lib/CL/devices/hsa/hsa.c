/* hsa.c - a minimalistic pocl device driver layer implementation

   Copyright (c) 2011-2013 Universidad Rey Juan Carlos and
                           Pekka Jääskeläinen / Tampere University of Technology
   
   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to deal
   in the Software without restriction, including without limitation the rights
   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   copies of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:
   
   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.
   
   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
   THE SOFTWARE.
*/

#include "hsa_header.h"
#include "cpuinfo.h"
#include "topology/pocl_topology.h"
#include "install-paths.h"
#include "common.h"

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <../dev_image.h>
#include <sys/time.h>

//Support Image APIs
const cl_image_format hsa_supported_image_formats[] = {
    { CL_R, CL_SNORM_INT8 },
    { CL_R, CL_SNORM_INT16 },
    { CL_R, CL_UNORM_INT8 },
    { CL_R, CL_UNORM_INT16 },
    { CL_R, CL_UNORM_SHORT_565 },
    { CL_R, CL_UNORM_SHORT_555 },
    { CL_R, CL_UNORM_INT_101010 },
    { CL_R, CL_SIGNED_INT8 },
    { CL_R, CL_SIGNED_INT16 },
    { CL_R, CL_SIGNED_INT32 },
    { CL_R, CL_UNSIGNED_INT8 },
    { CL_R, CL_UNSIGNED_INT16 },
    { CL_R, CL_UNSIGNED_INT32 },
    { CL_R, CL_HALF_FLOAT },
    { CL_R, CL_FLOAT },
    { CL_Rx, CL_SNORM_INT8 },
    { CL_Rx, CL_SNORM_INT16 },
    { CL_Rx, CL_UNORM_INT8 },
    { CL_Rx, CL_UNORM_INT16 },
    { CL_Rx, CL_UNORM_SHORT_565 },
    { CL_Rx, CL_UNORM_SHORT_555 },
    { CL_Rx, CL_UNORM_INT_101010 },
    { CL_Rx, CL_SIGNED_INT8 },
    { CL_Rx, CL_SIGNED_INT16 },
    { CL_Rx, CL_SIGNED_INT32 },
    { CL_Rx, CL_UNSIGNED_INT8 },
    { CL_Rx, CL_UNSIGNED_INT16 },
    { CL_Rx, CL_UNSIGNED_INT32 },
    { CL_Rx, CL_HALF_FLOAT },
    { CL_Rx, CL_FLOAT },
    { CL_A, CL_SNORM_INT8 },
    { CL_A, CL_SNORM_INT16 },
    { CL_A, CL_UNORM_INT8 },
    { CL_A, CL_UNORM_INT16 },
    { CL_A, CL_UNORM_SHORT_565 },
    { CL_A, CL_UNORM_SHORT_555 },
    { CL_A, CL_UNORM_INT_101010 },
    { CL_A, CL_SIGNED_INT8 },
    { CL_A, CL_SIGNED_INT16 },
    { CL_A, CL_SIGNED_INT32 },
    { CL_A, CL_UNSIGNED_INT8 },
    { CL_A, CL_UNSIGNED_INT16 },
    { CL_A, CL_UNSIGNED_INT32 },
    { CL_A, CL_HALF_FLOAT },
    { CL_A, CL_FLOAT },
    { CL_RG, CL_SNORM_INT8 },
    { CL_RG, CL_SNORM_INT16 },
    { CL_RG, CL_UNORM_INT8 },
    { CL_RG, CL_UNORM_INT16 },
    { CL_RG, CL_UNORM_SHORT_565 },
    { CL_RG, CL_UNORM_SHORT_555 },
    { CL_RG, CL_UNORM_INT_101010 },
    { CL_RG, CL_SIGNED_INT8 },
    { CL_RG, CL_SIGNED_INT16 },
    { CL_RG, CL_SIGNED_INT32 },
    { CL_RG, CL_UNSIGNED_INT8 },
    { CL_RG, CL_UNSIGNED_INT16 },
    { CL_RG, CL_UNSIGNED_INT32 },
    { CL_RG, CL_HALF_FLOAT },
    { CL_RG, CL_FLOAT },
    { CL_RGx, CL_SNORM_INT8 },
    { CL_RGx, CL_SNORM_INT16 },
    { CL_RGx, CL_UNORM_INT8 },
    { CL_RGx, CL_UNORM_INT16 },
    { CL_RGx, CL_UNORM_SHORT_565 },
    { CL_RGx, CL_UNORM_SHORT_555 },
    { CL_RGx, CL_UNORM_INT_101010 },
    { CL_RGx, CL_SIGNED_INT8 },
    { CL_RGx, CL_SIGNED_INT16 },
    { CL_RGx, CL_SIGNED_INT32 },
    { CL_RGx, CL_UNSIGNED_INT8 },
    { CL_RGx, CL_UNSIGNED_INT16 },
    { CL_RGx, CL_UNSIGNED_INT32 },
    { CL_RGx, CL_HALF_FLOAT },
    { CL_RGx, CL_FLOAT },
    { CL_RA, CL_SNORM_INT8 },
    { CL_RA, CL_SNORM_INT16 },
    { CL_RA, CL_UNORM_INT8 },
    { CL_RA, CL_UNORM_INT16 },
    { CL_RA, CL_UNORM_SHORT_565 },
    { CL_RA, CL_UNORM_SHORT_555 },
    { CL_RA, CL_UNORM_INT_101010 },
    { CL_RA, CL_SIGNED_INT8 },
    { CL_RA, CL_SIGNED_INT16 },
    { CL_RA, CL_SIGNED_INT32 },
    { CL_RA, CL_UNSIGNED_INT8 },
    { CL_RA, CL_UNSIGNED_INT16 },
    { CL_RA, CL_UNSIGNED_INT32 },
    { CL_RA, CL_HALF_FLOAT },
    { CL_RA, CL_FLOAT },
    { CL_RGBA, CL_SNORM_INT8 },
    { CL_RGBA, CL_SNORM_INT16 },
    { CL_RGBA, CL_UNORM_INT8 },
    { CL_RGBA, CL_UNORM_INT16 },
    { CL_RGBA, CL_UNORM_SHORT_565 },
    { CL_RGBA, CL_UNORM_SHORT_555 },
    { CL_RGBA, CL_UNORM_INT_101010 },
    { CL_RGBA, CL_SIGNED_INT8 },
    { CL_RGBA, CL_SIGNED_INT16 },
    { CL_RGBA, CL_SIGNED_INT32 },
    { CL_RGBA, CL_UNSIGNED_INT8 },
    { CL_RGBA, CL_UNSIGNED_INT16 },
    { CL_RGBA, CL_UNSIGNED_INT32 },
    { CL_RGBA, CL_HALF_FLOAT },
    { CL_RGBA, CL_FLOAT },
    { CL_INTENSITY, CL_UNORM_INT8 },
    { CL_INTENSITY, CL_UNORM_INT16 },
    { CL_INTENSITY, CL_SNORM_INT8 },
    { CL_INTENSITY, CL_SNORM_INT16 },
    { CL_INTENSITY, CL_HALF_FLOAT },
    { CL_INTENSITY, CL_FLOAT },
    { CL_LUMINANCE, CL_UNORM_INT8 },
    { CL_LUMINANCE, CL_UNORM_INT16 },
    { CL_LUMINANCE, CL_SNORM_INT8 },
    { CL_LUMINANCE, CL_SNORM_INT16 },
    { CL_LUMINANCE, CL_HALF_FLOAT },
    { CL_LUMINANCE, CL_FLOAT },
    { CL_RGB, CL_UNORM_SHORT_565 },
    { CL_RGB, CL_UNORM_SHORT_555 },
    { CL_RGB, CL_UNORM_INT_101010 },
    { CL_RGBx, CL_UNORM_SHORT_565 },
    { CL_RGBx, CL_UNORM_SHORT_555 },
    { CL_RGBx, CL_UNORM_INT_101010 },
    { CL_ARGB, CL_SNORM_INT8 },
    { CL_ARGB, CL_UNORM_INT8 },
    { CL_ARGB, CL_SIGNED_INT8 },
    { CL_ARGB, CL_UNSIGNED_INT8 },
    { CL_BGRA, CL_SNORM_INT8 },
    { CL_BGRA, CL_UNORM_INT8 },
    { CL_BGRA, CL_SIGNED_INT8 },
    { CL_BGRA, CL_UNSIGNED_INT8 }
 };


#define DUMMY_ARGS
/*
* Determines if a memory region can be used for kernarg
* allocations.
*/
static hsa_status_t get_kernarg(hsa_region_t region, void* data) {
	hsa_region_flag_t flags;
	hsa_region_get_info(region, HSA_REGION_INFO_FLAGS, &flags);
	if (flags & HSA_REGION_FLAG_KERNARG) {
	hsa_region_t* ret = (hsa_region_t*) data;
	*ret = region;
	}
	return HSA_STATUS_SUCCESS;
}

#define max(a,b) (((a) > (b)) ? (a) : (b))

#define COMMAND_LENGTH 2048
#define WORKGROUP_STRING_LENGTH 128

struct data {
  /* Currently loaded kernel. */
  cl_kernel current_kernel;
  /* Loaded kernel dynamic library handle. */
  lt_dlhandle current_dlhandle;
};

void
pocl_hsa_init (cl_device_id device, const char* parameters)
{
  struct data *d;
  
  d = (struct data *) malloc (sizeof (struct data));
  
  d->current_kernel = NULL;
  d->current_dlhandle = 0;
  device->data = d;

}

//加上hsa_memort_register
void *
pocl_hsa_malloc (void *device_data, cl_mem_flags flags,
		    size_t size, void *host_ptr)
{
  void *b;
  struct data* d = (struct data*)device_data;
  hsa_status_t err;
  if (flags & CL_MEM_COPY_HOST_PTR)
    {
      if (posix_memalign (&b, MAX_EXTENDED_ALIGNMENT, size) == 0)
        {
          memcpy (b, host_ptr, size);
          err = hsa_memory_register(b, size);
          return b;
        }
      
      return NULL;
    }
  
  if ((flags & CL_MEM_USE_HOST_PTR) && host_ptr != NULL)
    {
      err = hsa_memory_register(host_ptr, size);
	  return host_ptr;
    }

  if (posix_memalign (&b, MAX_EXTENDED_ALIGNMENT, size) == 0){
	  err = hsa_memory_register(b, size);
	  memset(b, 0, size);
	  return b;
  }
  return NULL;
}

void *
pocl_hsa_create_sub_buffer (void* data, void* ptr, size_t origin, size_t size)
{
	hsa_status_t err;
	void* b;
	b = malloc(size);
	memset(b, 0, size);
	memcpy(ptr + origin, b, size);
	err = hsa_memory_register(ptr + origin, size);
	free(b);
	return ptr + origin;
}

void
pocl_hsa_free (void *data, cl_mem_flags flags, void *ptr)
{
  if (flags & CL_MEM_USE_HOST_PTR)
	  return;
  free (ptr);
}

void
pocl_hsa_read (void *data, void *host_ptr, const void *device_ptr, size_t cb)
{
  if (host_ptr == device_ptr)
    return;

  memcpy (host_ptr, device_ptr, cb);
}

void
pocl_hsa_write (void *data, const void *host_ptr, void *device_ptr, size_t cb)
{
  if (host_ptr == device_ptr)
    return;

  memcpy (device_ptr, host_ptr, cb);
}

void
pocl_hsa_fill (void* data, const void* pattern, size_t pattern_size, void* device_ptr, size_t cb)
{
	int i;
	void* pointer = device_ptr;
	for(i=0 ; i < cb/pattern_size; i++){
		memcpy (pointer, pattern, pattern_size);
		pointer += pattern_size;
	}
}

void
pocl_hsa_run 
(void *data, 
 _cl_command_node* cmd)
{
  struct data *d;
  int error;
  const char *module_fn;
  char command[COMMAND_LENGTH];
  char workgroup_string[WORKGROUP_STRING_LENGTH];
  struct pocl_argument *al;
  unsigned device, mem_index;

  size_t x, y, z;
  unsigned i;
  pocl_workgroup w;
  char* tmpdir = cmd->command.run.tmp_dir;
  cl_kernel kernel = cmd->command.run.kernel;


  struct pocl_context *pc = &cmd->command.run.pc;

  assert (data != NULL);
  d = (struct data *) data;
  d->current_kernel = kernel;

  /* Find which device number within the context correspond
     to current device.  */
  for (i = 0; i < kernel->context->num_devices; ++i)
    {
      if (kernel->context->devices[i]->type &CL_DEVICE_TYPE_GPU)
        {
          mem_index = kernel->context->devices[i]->dev_id;
          device = i;
          break;
        }
    }
#ifdef DEBUG
  	  printf("%d %d\n",device, mem_index);
#endif
	  /*
	   * Create a signal to wait for the dispatch to finish.
	   */
	  hsa_signal_t signal;
	  hsa_status_t err=hsa_signal_create(1, 0, NULL, &signal);
#ifndef DEBUG
  if(err != HSA_STATUS_SUCCESS)
#endif
	  check(Creating a HSA signal, err);
	  /*
	   * Initialize the dispatch packet.
	   */
	  hsa_dispatch_packet_t aql;
	  memset(&aql, 0, sizeof(aql));

	  int group_size = 0;

	  unsigned int num_args = kernel->num_args;

	  /*
	   * Setup the dispatch information.
	   */
	  aql.completion_signal=signal;
	  aql.dimensions=(uint16_t)pc->work_dim;
	  aql.workgroup_size_x=(uint16_t)pc->local_size[0];
	  aql.workgroup_size_y=(uint16_t)pc->local_size[1];
	  aql.workgroup_size_z=(uint16_t)pc->local_size[2];
	  aql.grid_size_x=(uint16_t)pc->global_size[0];
	  aql.grid_size_y=(uint16_t)pc->global_size[1];
	  aql.grid_size_z=(uint16_t)pc->global_size[2];
	  aql.header.type=HSA_PACKET_TYPE_DISPATCH;
	  aql.header.acquire_fence_scope=2;
	  aql.header.release_fence_scope=2;
	  aql.header.barrier=1;
	  aql.group_segment_size = kernel->hsaCodeDescriptor->workgroup_group_segment_byte_size;
	  aql.private_segment_size = kernel->hsaCodeDescriptor->workitem_private_segment_byte_size;

	  size_t kernel_arg_buffer_size = kernel->hsaCodeDescriptor->kernarg_segment_byte_size;
#ifdef DEBUG
	  for(i=0; i<kernel->num_args; i++)
		  printf("%d, ",kernel->arg_is_pointer[i]);
	  printf("\n");
	  for(i=0; i<kernel->num_args; i++)
		  printf("%d, ",kernel->arg_is_local[i]);
	  printf("\n");
	  /*for(i=0; i<kernel->num_args; i++)
		  printf("%d, ",kernel->arg_is_image[i]);
	  printf("\n");
	  for(i=0; i<kernel->num_args; i++)
		  printf("%d, ",kernel->arg_is_sampler[i]);
	  printf("\n");*/
	  //}
#endif
	  void* args __attribute__ ((aligned(HSA_ARGUMENT_ALIGN_BYTES))) = malloc(kernel_arg_buffer_size) ;
	  memset(args, 0, kernel_arg_buffer_size);
//by ccchen, we should access the content of address value instead of value.
	  void* args_pointer = args;
	  void* local_pointer = kernel->hsaCodeDescriptor->workgroup_group_segment_byte_size;
	  args_pointer += sizeof(void*) * (kernel->hsaArgCount - kernel->num_args);
	  if(kernel->num_args > 0){
//當struct用typedef替代，就沒辦法用抓字串的方式取得了
		  if(strcmp(kernel->name,"calPriceVega") == 0){
			  memcpy(args_pointer, &(kernel->dyn_arguments[0].value), sizeof(void*));
			  args_pointer += sizeof(void*);
			  for(i=1; i<num_args; i++){
				  if(kernel->arg_is_pointer[i]){// by ccchen, in case array
					  if((uint64_t)(args_pointer-args)%8 !=0)
						  args_pointer += 8 - (uint64_t)(args_pointer-args)%8;
					  if(kernel->dyn_arguments[i].value == NULL){// by ccchen, in case __local array
						  void* p = local_pointer;
						  memcpy(args_pointer, &p, sizeof(void*));
						  args_pointer += sizeof(void*);
						  local_pointer += sizeof(void*);
						  aql.group_segment_size += kernel->dyn_arguments[i].size;
					  }else{// by ccchen, in case cl_mem array
						  void* p = (*(cl_mem*)kernel->dyn_arguments[i].value)->device_ptrs[mem_index];
						  memcpy(args_pointer, &p, sizeof(void*));
						  args_pointer += sizeof(void*);
					  }
				  }else{ // by ccchen, in case scalar
					  if(kernel->arg_is_struct[i]){
						  memcpy(args_pointer, &(kernel->dyn_arguments[i].value), sizeof(void*));
						  args_pointer += sizeof(void*);
					  }else{
						  memcpy(args_pointer, kernel->dyn_arguments[i].value, kernel->dyn_arguments[i].size);
						  args_pointer += kernel->dyn_arguments[i].size;
					  }
				  }
			  }
		  }
	/*	  else if(strcmp(kernel->name,"icvFindMaximaInLayer") == 0){
	#if 1
			  printf("Test for icvFindMaximaInLayer\n");
	#endif
			  memcpy(args_pointer, (kernel->dyn_arguments[0].value), sizeof(void*));
			  args_pointer += sizeof(void*);
			  memcpy(args_pointer, (kernel->dyn_arguments[1].value), sizeof(void*));
			  args_pointer += sizeof(void*);
			  memcpy(args_pointer, (kernel->dyn_arguments[2].value), sizeof(void*));
			  args_pointer += sizeof(void*);
			  memcpy(args_pointer, (kernel->dyn_arguments[3].value), sizeof(void*));
			  args_pointer += sizeof(void*);
			  for(i=4; i<num_args; i++){
				  if(kernel->arg_is_pointer[i]){// by ccchen, in case array
					  if((uint64_t)(args_pointer-args)%8 !=0)
						  args_pointer += 8 - (uint64_t)(args_pointer-args)%8;
					  if(kernel->dyn_arguments[i].value == NULL){// by ccchen, in case __local array
						  void* p = local_pointer;
						  memcpy(args_pointer, &p, sizeof(void*));
						  args_pointer += sizeof(void*);
						  local_pointer += sizeof(void*);
						  aql.group_segment_size += kernel->dyn_arguments[i].size;
					  }else{// by ccchen, in case cl_mem array
						  void* p = (*(cl_mem*)kernel->dyn_arguments[i].value)->device_ptrs[mem_index];
						  memcpy(args_pointer, &p, sizeof(void*));
						  args_pointer += sizeof(void*);
					  }
				  }else{ // by ccchen, in case scalar
					  if(kernel->arg_is_struct[i]){
						  memcpy(args_pointer, &(kernel->dyn_arguments[i].value), sizeof(void*));
						  args_pointer += sizeof(void*);
					  }else{
						  memcpy(args_pointer, kernel->dyn_arguments[i].value, kernel->dyn_arguments[i].size);
						  args_pointer += kernel->dyn_arguments[i].size;
					  }
				  }
			  }
		  }*/
//general case
		  else{
			  for(i=0; i<num_args; i++){
				  al = &(cmd->command.run.arguments[i]);
				  if(kernel->arg_is_local[i]){
//Fix alignment issue
					  if((uint64_t)(args_pointer-args)%8 !=0)
						  args_pointer += 8 - (uint64_t)(args_pointer-args)%8;
//End of alignment issue

//Local argument issue
//Refer to https://github.com/HSAFoundation/HSA-Runtime-AMD/issues/8
					  void* p = local_pointer;
					  memcpy(args_pointer, &p, sizeof(void*));
					  args_pointer += sizeof(void*);
					  local_pointer += al->size;
					  aql.group_segment_size += kernel->dyn_arguments[i].size;
				  }else if(kernel->arg_is_pointer[i]){// by ccchen, in case array
					  if((uint64_t)(args_pointer-args)%8 !=0)
						  args_pointer += 8 - (uint64_t)(args_pointer-args)%8;
//In case of Null array
					  if(al->value == NULL){
						  void* p = NULL;
						  memcpy(args_pointer, &p, sizeof(void*));
						  args_pointer += sizeof(void*);
					  }else{
// by ccchen, in case cl_mem array
						  //取出__global *ptr的address
						  void* p = ((*(cl_mem *) (al->value))->device_ptrs[mem_index]);
							//把address的值複製到argument buffer
						  memcpy(args_pointer, &p, sizeof(void*));
						  args_pointer += sizeof(void*);
					  }
				  }else if(kernel->arg_is_image[i]){
#ifdef DEBUG
					  printf("Entering image init\n");
#endif
					  if((uint64_t)(args_pointer-args)%8 !=0)
						  args_pointer += 8 - (uint64_t)(args_pointer-args)%8;
					  hsa_ext_image_handle_t p = (hsa_ext_image_handle_t)(*(cl_mem *)(al->value))->image_handle;
					  memcpy(args_pointer, &p, sizeof(uint64_t));
					  args_pointer += sizeof(uint64_t);
				  }else{ // by ccchen, in case scalar
//For struct argument
//HSAIL把struct當一般argument處理
					  if(kernel->arg_is_struct[i]){
						  if((uint64_t)(args_pointer-args)%8 !=0)
							  args_pointer += 8 - (uint64_t)(args_pointer-args)%8;
						  memcpy(args_pointer, &(al->value), sizeof(void*));
						  args_pointer += sizeof(void*);
					  }else{
						  memcpy(args_pointer, al->value, al->size);
						  args_pointer += al->size;
					  }
				  }
			  }
		  }
	  }
#ifdef DEBUG
	  printf("kernel_name: %s\n kernel size: %lu\n group size: %d\n",kernel->name, kernel_arg_buffer_size, aql.group_segment_size);
#endif
	  /*
	   * Find a memory region that supports kernel arguments.
	   */
	  hsa_region_t kernarg_region = 0;

	  hsa_agent_iterate_regions(kernel->context->devices[device]->agent_id, get_kernarg, &kernarg_region);
	  err = (kernarg_region == 0) ? HSA_STATUS_ERROR : HSA_STATUS_SUCCESS;
#ifndef DEBUG
  if(err != HSA_STATUS_SUCCESS)
#endif
	  check(Finding a kernarg memory region, err);
  	  void* kernel_arg_buffer;
	  /*
	   * Allocate the kernel argument buffer from the correct region.
	   */
	  err = hsa_memory_allocate(kernarg_region, kernel_arg_buffer_size,
						  &kernel_arg_buffer);
#ifndef DEBUG
  if(err != HSA_STATUS_SUCCESS)
#endif
	  check(Allocating kernel argument memory buffer, err);

       memcpy(kernel_arg_buffer, args, kernel_arg_buffer_size);
       /*
	   * Bind kernel code and the kernel argument buffer to the
	   * aql packet.
	   */
	  aql.kernel_object_address=kernel->hsaCodeDescriptor->code.handle;
	  aql.kernarg_address=(uint64_t)kernel_arg_buffer;

	 /*
	 * Register the memory region for the argument buffer.
	 */
	 err = hsa_memory_register(kernel_arg_buffer, kernel_arg_buffer_size);
#ifndef DEBUG
  if(err != HSA_STATUS_SUCCESS)
#endif
	 check(Registering the argument buffer, err);
	  /*
	   * Obtain the current queue write index.
	   */
	  uint64_t index = hsa_queue_load_write_index_relaxed(cmd->command.run.command_queue->queue);

	  /*
	   * Write the aql packet at the calculated queue index address.
	   */
	  const uint32_t queueMask = cmd->command.run.command_queue->queue->size - 1;
	  ((hsa_dispatch_packet_t*)(cmd->command.run.command_queue->queue->base_address))[index&queueMask]=aql;

	  /*
	   * Increment the write index and ring the doorbell to dispatch the kernel.
	   */
	  hsa_queue_store_write_index_relaxed(cmd->command.run.command_queue->queue, index+1);
	  hsa_signal_store_relaxed(cmd->command.run.command_queue->queue->doorbell_signal, index);
#ifndef DEBUG
  if(err != HSA_STATUS_SUCCESS)
#endif
	  check(Dispatching the kernel, err);

	  /*
	   * Wait on the dispatch signal until the kernel is finished.
	   */
	  hsa_signal_t finish_signal = hsa_signal_wait_acquire(signal, HSA_LT, 1, (uint64_t)-1, HSA_WAIT_EXPECTANCY_UNKNOWN);
	  if(finish_signal != 0){
		  hsa_shut_down();
		  printf("kernel execution failed!!\n");
		  return;
	  }
#ifndef DEBUG
	  if(err != HSA_STATUS_SUCCESS)
#endif
	  check(Waitng on the dispatch signal, err);
	  #ifndef DEBUG
	  	  if(err != HSA_STATUS_SUCCESS)
	  #endif
	  check(Destroying the signal, err);
	  err=hsa_memory_free(kernel_arg_buffer);
	  #ifndef DEBUG
	  	  if(err != HSA_STATUS_SUCCESS)
	  #endif
	  check(hsa_memory_free, err);
}

void
pocl_hsa_copy (void *data, const void *src_ptr, void *__restrict__ dst_ptr, size_t cb)
{
  if (src_ptr == dst_ptr)
    return;
  
  memcpy (dst_ptr, src_ptr, cb);
}

void
pocl_hsa_copy_rect (void *data,
                      const void *__restrict const src_ptr,
                      void *__restrict__ const dst_ptr,
                      const size_t *__restrict__ const src_origin,
                      const size_t *__restrict__ const dst_origin, 
                      const size_t *__restrict__ const region,
                      size_t const src_row_pitch,
                      size_t const src_slice_pitch,
                      size_t const dst_row_pitch,
                      size_t const dst_slice_pitch)
{
  char const *__restrict const adjusted_src_ptr = 
    (char const*)src_ptr +
    src_origin[0] + src_row_pitch * (src_origin[1] + src_slice_pitch * src_origin[2]);
  char *__restrict__ const adjusted_dst_ptr = 
    (char*)dst_ptr +
    dst_origin[0] + dst_row_pitch * (dst_origin[1] + dst_slice_pitch * dst_origin[2]);
  
  size_t j, k;

  /* TODO: handle overlaping regions */
  
  for (k = 0; k < region[2]; ++k)
    for (j = 0; j < region[1]; ++j)
      memcpy (adjusted_dst_ptr + dst_row_pitch * j + dst_slice_pitch * k,
              adjusted_src_ptr + src_row_pitch * j + src_slice_pitch * k,
              region[0]);
}

void
pocl_hsa_write_rect (void *data,
                       const void *__restrict__ const host_ptr,
                       void *__restrict__ const device_ptr,
                       const size_t *__restrict__ const buffer_origin,
                       const size_t *__restrict__ const host_origin, 
                       const size_t *__restrict__ const region,
                       size_t const buffer_row_pitch,
                       size_t const buffer_slice_pitch,
                       size_t const host_row_pitch,
                       size_t const host_slice_pitch)
{
  char *__restrict const adjusted_device_ptr = 
    (char*)device_ptr +
    buffer_origin[0] + buffer_row_pitch * (buffer_origin[1] + buffer_slice_pitch * buffer_origin[2]);
  char const *__restrict__ const adjusted_host_ptr = 
    (char const*)host_ptr +
    host_origin[0] + host_row_pitch * (host_origin[1] + host_slice_pitch * host_origin[2]);
  
  size_t j, k;

  /* TODO: handle overlaping regions */
  
  for (k = 0; k < region[2]; ++k)
    for (j = 0; j < region[1]; ++j)
      memcpy (adjusted_device_ptr + buffer_row_pitch * j + buffer_slice_pitch * k,
              adjusted_host_ptr + host_row_pitch * j + host_slice_pitch * k,
              region[0]);
}

void
pocl_hsa_read_rect (void *data,
                      void *__restrict__ const host_ptr,
                      void *__restrict__ const device_ptr,
                      const size_t *__restrict__ const buffer_origin,
                      const size_t *__restrict__ const host_origin, 
                      const size_t *__restrict__ const region,
                      size_t const buffer_row_pitch,
                      size_t const buffer_slice_pitch,
                      size_t const host_row_pitch,
                      size_t const host_slice_pitch)
{
  char const *__restrict const adjusted_device_ptr = 
    (char const*)device_ptr +
    buffer_origin[0] + buffer_row_pitch * (buffer_origin[1] + buffer_slice_pitch * buffer_origin[2]);
  char *__restrict__ const adjusted_host_ptr = 
    (char*)host_ptr +
    host_origin[0] + host_row_pitch * (host_origin[1] + host_slice_pitch * host_origin[2]);
  
  size_t j, k;
  
  /* TODO: handle overlaping regions */
  
  for (k = 0; k < region[2]; ++k)
    for (j = 0; j < region[1]; ++j)
      memcpy (adjusted_host_ptr + host_row_pitch * j + host_slice_pitch * k,
              adjusted_device_ptr + buffer_row_pitch * j + buffer_slice_pitch * k,
              region[0]);
}


void *
pocl_hsa_map_mem (void *data, void *buf_ptr, 
                      size_t offset, size_t size,
                      void *host_ptr) 
{
  /* All global pointers of the pthread/CPU device are in 
     the host address space already, and up to date. */
  if (host_ptr != NULL) return host_ptr;
  return buf_ptr + offset;
}

void
pocl_hsa_uninit (cl_device_id device)
{
  struct data *d = (struct data*)device->data;
  free (d);
  device->data = NULL;
}

cl_ulong
pocl_hsa_get_timer_value (void *data) 
{
  struct timeval current;
  gettimeofday(&current, NULL);  
  return (current.tv_sec * 1000000 + current.tv_usec)*1000;
}

cl_int
pocl_hsa_get_supported_image_formats (cl_mem_flags flags,
                                        const cl_image_format **image_formats,
                                        cl_int *num_img_formats)
{
    if (num_img_formats == NULL || image_formats == NULL)
      return CL_INVALID_VALUE;

    *num_img_formats = sizeof(hsa_supported_image_formats)/sizeof(cl_image_format);
    *image_formats = hsa_supported_image_formats;

    return CL_SUCCESS;
}
