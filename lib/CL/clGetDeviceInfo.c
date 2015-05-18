/* OpenCL runtime library: clGetDeviceInfo()

   Copyright (c) 2011-2012 Kalle Raiskila and Pekka Jääskeläinen
   
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

#include "pocl_cl.h"
#include "pocl_util.h"
#include <string.h>
#include <hsa.h>

/* A version for querying the info and in case the device returns 
   a zero, assume the device info query hasn't been implemented 
   for the device driver at hand. Warns about an incomplete 
   implementation. */

#define POCL_RETURN_DEVICE_INFO_WITH_IMPL_CHECK(__TYPE__, __VALUE__)   \
 {                                                                  \
    size_t const value_size = sizeof(__TYPE__);                     \
    if (param_value)                                                \
      {                                                             \
        if (param_value_size < value_size) return CL_INVALID_VALUE; \
        *(__TYPE__*)param_value = __VALUE__;                        \
        if (__VALUE__ == 0) POCL_WARN_INCOMPLETE();                 \
      }                                                             \
    if (param_value_size_ret)                                       \
      *param_value_size_ret = value_size;                           \
    return CL_SUCCESS;                                              \
  }

#define POCL_RETURN_DEVICE_INFO_STR(__STR__)                        \
  {                                                                 \
    size_t const value_size = strlen(__STR__) + 1;                  \
    if (param_value)                                                \
      {                                                             \
        if (param_value_size < value_size) return CL_INVALID_VALUE; \
        memcpy(param_value, __STR__, value_size);                   \
      }                                                             \
    if (param_value_size_ret)                                       \
      *param_value_size_ret = value_size;                           \
    return CL_SUCCESS;                                              \
  }
    

  
CL_API_ENTRY cl_int CL_API_CALL
POname(clGetDeviceInfo)(cl_device_id   device,
                cl_device_info param_name, 
                size_t         param_value_size, 
                void *         param_value,
                size_t *       param_value_size_ret) CL_API_SUFFIX__VERSION_1_0
{
hsa_status_t err;
switch (param_name)
{
	case CL_DEVICE_IMAGE_SUPPORT:
		POCL_RETURN_GETINFO(cl_bool, CL_TRUE);
		break;
	case CL_DEVICE_TYPE:
	{
#if defined HSA_RUNTIME
		if(device->type&CL_DEVICE_TYPE_GPU){
			hsa_device_type_t device_type;
			err = hsa_agent_get_info(device->agent_id, HSA_AGENT_INFO_DEVICE, &device_type);
			switch(device_type){
				case HSA_DEVICE_TYPE_CPU:
					device->type = CL_DEVICE_TYPE_CPU;
					break;
				case HSA_DEVICE_TYPE_GPU:
					device->type = CL_DEVICE_TYPE_GPU;
					break;
				case HSA_DEVICE_TYPE_DSP: //TODO: pocl not support!
					//device->type = CL_DEVICE_TYPE_DSP;
					break;
				default:
					device->type = CL_DEVICE_TYPE_GPU;
					break;
		}
	  }
#endif
	  POCL_RETURN_GETINFO(cl_device_type, device->type);
	}	
	case CL_DEVICE_VENDOR_ID:
	  POCL_RETURN_GETINFO(cl_uint, device->vendor_id);
	case CL_DEVICE_MAX_COMPUTE_UNITS:
		POCL_RETURN_GETINFO(cl_uint, device->max_compute_units);
	case CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS          :
	{
#if defined HSA_RUNTIME
	if(device->type&CL_DEVICE_TYPE_GPU){
		  hsa_dim3_t grid_max_dim;
		  err = hsa_agent_get_info(device->agent_id, HSA_AGENT_INFO_GRID_MAX_DIM, &grid_max_dim);
		  if(grid_max_dim.z != 0){
			  device->max_work_item_dimensions = 3;
		  }else if(grid_max_dim.y != 0){
			  device->max_work_item_dimensions = 2;
		  }else if(grid_max_dim.x != 0){
			  device->max_work_item_dimensions = 1;
		  }
	  }
#endif
	  POCL_RETURN_GETINFO(cl_uint, device->max_work_item_dimensions);
	}	
	case CL_DEVICE_MAX_WORK_ITEM_SIZES:
	{
		typedef struct { size_t size[3]; } size_t_3;
#if defined HSA_RUNTIME
		if(device->type&CL_DEVICE_TYPE_GPU){
			typedef struct { size_t size[3]; } size_t_3;
			hsa_dim3_t grid_max_dim;
			err = hsa_agent_get_info(device->agent_id, HSA_AGENT_INFO_GRID_MAX_DIM, &grid_max_dim);
			device->max_work_item_sizes[0] = grid_max_dim.x;
			device->max_work_item_sizes[1] = grid_max_dim.y;
			device->max_work_item_sizes[2] = grid_max_dim.z;
		}
#endif
		POCL_RETURN_GETINFO(size_t_3, *(size_t_3 const *)device->max_work_item_sizes);
	}
	case CL_DEVICE_MAX_WORK_GROUP_SIZE               :
	{
#if defined HSA_RUNTIME
		if(device->type&CL_DEVICE_TYPE_GPU){
			uint32_t wg_max_size = 0;
			err = hsa_agent_get_info(device->agent_id, HSA_AGENT_INFO_WORKGROUP_MAX_SIZE, &wg_max_size);
			device->max_work_group_size = wg_max_size;
		}else
#endif
		{
			size_t max_wg_size = device->max_work_group_size;
			if (getenv ("POCL_MAX_WORK_GROUP_SIZE") != NULL)
			{
				size_t from_env = atoi (getenv ("POCL_MAX_WORK_GROUP_SIZE"));
			    if (from_env < max_wg_size) max_wg_size = from_env;
			}
		}
		POCL_RETURN_GETINFO(size_t, device->max_work_group_size);
	 }
	case CL_DEVICE_PREFERRED_VECTOR_WIDTH_CHAR: //TODO: by ccchen, Not Support Yet!!
	  POCL_RETURN_DEVICE_INFO_WITH_IMPL_CHECK(cl_uint, device->preferred_vector_width_char);
	case CL_DEVICE_PREFERRED_VECTOR_WIDTH_SHORT: //TODO: by ccchen, Not Support Yet!!
	  POCL_RETURN_DEVICE_INFO_WITH_IMPL_CHECK(cl_uint, device->preferred_vector_width_short);
	case CL_DEVICE_PREFERRED_VECTOR_WIDTH_INT: //TODO: by ccchen, Not Support Yet!!
	  POCL_RETURN_DEVICE_INFO_WITH_IMPL_CHECK(cl_uint, device->preferred_vector_width_int);
	case CL_DEVICE_PREFERRED_VECTOR_WIDTH_LONG: //TODO: by ccchen, Not Support Yet!!
	  POCL_RETURN_DEVICE_INFO_WITH_IMPL_CHECK(cl_uint, device->preferred_vector_width_long);
	case CL_DEVICE_PREFERRED_VECTOR_WIDTH_FLOAT: //TODO: by ccchen, Not Support Yet!!
	  POCL_RETURN_DEVICE_INFO_WITH_IMPL_CHECK(cl_uint, device->preferred_vector_width_float);
	case CL_DEVICE_PREFERRED_VECTOR_WIDTH_DOUBLE: //TODO: by ccchen, Not Support Yet!!
	  POCL_RETURN_DEVICE_INFO_WITH_IMPL_CHECK(cl_uint, device->preferred_vector_width_double);
	case CL_DEVICE_MAX_CLOCK_FREQUENCY               ://by clinfo
	  POCL_RETURN_DEVICE_INFO_WITH_IMPL_CHECK(cl_uint, device->max_clock_frequency);
	case CL_DEVICE_ADDRESS_BITS                      ://TODO: by ccchen, Not Support Yet!!
	  POCL_RETURN_DEVICE_INFO_WITH_IMPL_CHECK(cl_uint, device->address_bits);
	case CL_DEVICE_MAX_READ_IMAGE_ARGS               ://TODO: by ccchen, Not Support Yet!!
	  POCL_RETURN_DEVICE_INFO_WITH_IMPL_CHECK(cl_uint, device->max_read_image_args);
	case CL_DEVICE_MAX_WRITE_IMAGE_ARGS              ://TODO: by ccchen, Not Support Yet!!
	  POCL_RETURN_DEVICE_INFO_WITH_IMPL_CHECK(cl_uint, device->max_write_image_args);
	case CL_DEVICE_MAX_MEM_ALLOC_SIZE: //by clinfo
	  POCL_RETURN_DEVICE_INFO_WITH_IMPL_CHECK(cl_ulong, device->max_mem_alloc_size);
	case CL_DEVICE_IMAGE2D_MAX_WIDTH                 :
	{	
#if defined HSA_RUNTIME
	  if(device->type&CL_DEVICE_TYPE_GPU){
		hsa_dim3_t image2d_max_dim;
		err = hsa_agent_get_info(device->agent_id, HSA_EXT_AGENT_INFO_IMAGE2D_MAX_DIM,&image2d_max_dim);
		device->image2d_max_width = image2d_max_dim.x;
	  }
#endif
	  POCL_RETURN_DEVICE_INFO_WITH_IMPL_CHECK(size_t, device->image2d_max_width);
	}
	case CL_DEVICE_IMAGE2D_MAX_HEIGHT                :
	{
#if defined HSA_RUNTIME
	if(device->type&CL_DEVICE_TYPE_GPU){
		  hsa_dim3_t image2d_max_dim;
		  err = hsa_agent_get_info(device->agent_id, HSA_EXT_AGENT_INFO_IMAGE2D_MAX_DIM, &image2d_max_dim);
		  device->image2d_max_height = image2d_max_dim.y;
	  }
#endif
	  POCL_RETURN_DEVICE_INFO_WITH_IMPL_CHECK(size_t, device->image2d_max_height);
	}	
	case CL_DEVICE_IMAGE3D_MAX_WIDTH                 :
	{
#if defined HSA_RUNTIME
	 if(device->type&CL_DEVICE_TYPE_GPU){
		  hsa_dim3_t image3d_max_dim;
		  err = hsa_agent_get_info(device->agent_id, HSA_EXT_AGENT_INFO_IMAGE3D_MAX_DIM, &image3d_max_dim);
		  device->image3d_max_width = image3d_max_dim.x;
	  }
#endif
	  POCL_RETURN_DEVICE_INFO_WITH_IMPL_CHECK(size_t, device->image3d_max_width);
	}
	case CL_DEVICE_IMAGE3D_MAX_HEIGHT                :
	{
#if defined HSA_RUNTIME
	  if(device->type&CL_DEVICE_TYPE_GPU){
		  hsa_dim3_t image3d_max_dim;
	  	  err = hsa_agent_get_info(device->agent_id, HSA_EXT_AGENT_INFO_IMAGE3D_MAX_DIM, &image3d_max_dim);
	  	  device->image3d_max_height = image3d_max_dim.y;
	  }
#endif
	  POCL_RETURN_DEVICE_INFO_WITH_IMPL_CHECK(size_t, device->image3d_max_height);
	}
	case CL_DEVICE_IMAGE3D_MAX_DEPTH                 :
	{
#if defined HSA_RUNTIME
	  if(device->type&CL_DEVICE_TYPE_GPU){
		  hsa_dim3_t image3d_max_dim;
		  err = hsa_agent_get_info(device->agent_id, HSA_EXT_AGENT_INFO_IMAGE3D_MAX_DIM, &image3d_max_dim);
		  device->image3d_max_depth = image3d_max_dim.z;
	  }
#endif
	  POCL_RETURN_DEVICE_INFO_WITH_IMPL_CHECK(size_t, device->image3d_max_depth);
	}
	case CL_DEVICE_MAX_PARAMETER_SIZE                ://by clinfo
	  POCL_RETURN_DEVICE_INFO_WITH_IMPL_CHECK(size_t, device->max_parameter_size);
	case CL_DEVICE_MAX_SAMPLERS                      :
	{
#if defined HSA_RUNTIME
	  if(device->type&CL_DEVICE_TYPE_GPU){
		  uint32_t sampler_max = 0;
		  err = hsa_agent_get_info(device->agent_id, HSA_EXT_AGENT_INFO_SAMPLER_MAX, &sampler_max);
		  device->max_samplers = sampler_max;
	  }
#endif
	  POCL_RETURN_DEVICE_INFO_WITH_IMPL_CHECK(cl_uint, device->max_samplers);
	}
	case CL_DEVICE_MEM_BASE_ADDR_ALIGN               ://by clinfo
	  POCL_RETURN_DEVICE_INFO_WITH_IMPL_CHECK(cl_uint, device->mem_base_addr_align);
	case CL_DEVICE_MIN_DATA_TYPE_ALIGN_SIZE          ://by clinfo
	  POCL_RETURN_DEVICE_INFO_WITH_IMPL_CHECK(cl_uint, device->min_data_type_align_size);
	case CL_DEVICE_SINGLE_FP_CONFIG                  ://TODO: by ccchen, Not Support Yet!!
	  POCL_RETURN_DEVICE_INFO_WITH_IMPL_CHECK(cl_ulong, device->single_fp_config);
	case CL_DEVICE_GLOBAL_MEM_CACHE_TYPE             ://TODO: by ccchen, Not Support Yet!!
	  POCL_RETURN_GETINFO(cl_uint, device->global_mem_cache_type);
	case CL_DEVICE_GLOBAL_MEM_CACHELINE_SIZE         ://by clinfo
	  POCL_RETURN_GETINFO(cl_uint, device->global_mem_cacheline_size);
	case CL_DEVICE_GLOBAL_MEM_CACHE_SIZE             ://by clinfo
	  POCL_RETURN_GETINFO(cl_ulong, device->global_mem_cache_size);
	case CL_DEVICE_GLOBAL_MEM_SIZE://by clinfo
	  POCL_RETURN_DEVICE_INFO_WITH_IMPL_CHECK(cl_ulong, device->global_mem_size);
	case CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE          ://by clinfo
	  POCL_RETURN_DEVICE_INFO_WITH_IMPL_CHECK(cl_uint, device->max_constant_buffer_size);
	case CL_DEVICE_MAX_CONSTANT_ARGS                 ://by clinfo
	  POCL_RETURN_DEVICE_INFO_WITH_IMPL_CHECK(cl_uint, device->max_constant_args);
	case CL_DEVICE_LOCAL_MEM_TYPE                    ://by clinfo
	  POCL_RETURN_DEVICE_INFO_WITH_IMPL_CHECK(cl_uint, device->local_mem_type);
	case CL_DEVICE_LOCAL_MEM_SIZE: //by clinfo
	  POCL_RETURN_DEVICE_INFO_WITH_IMPL_CHECK(cl_ulong, device->local_mem_size);
	case CL_DEVICE_ERROR_CORRECTION_SUPPORT          : //by clinfo
	  POCL_RETURN_GETINFO(cl_bool, device->error_correction_support);
	case CL_DEVICE_PROFILING_TIMER_RESOLUTION        : //by clinfo
	  POCL_RETURN_GETINFO(cl_uint, device->profiling_timer_resolution);
	case CL_DEVICE_ENDIAN_LITTLE                     : //by clinfo
	  POCL_RETURN_GETINFO(cl_uint, device->endian_little);
	case CL_DEVICE_AVAILABLE                         : //CL_TRUE by default
	  POCL_RETURN_GETINFO(cl_uint, device->available);
	case CL_DEVICE_COMPILER_AVAILABLE                : //CL_TRUE by default
	  POCL_RETURN_GETINFO(cl_uint, device->compiler_available);
	case CL_DEVICE_EXECUTION_CAPABILITIES            : //Not for sure!!
	  POCL_RETURN_GETINFO(cl_uint, device->execution_capabilities);
	case CL_DEVICE_QUEUE_PROPERTIES                  : //by clinfo
	  POCL_RETURN_GETINFO(cl_uint, device->queue_properties);

	case CL_DEVICE_NAME                              :
	{
#if defined HSA_RUNTIME
	if(device->type&CL_DEVICE_TYPE_GPU){
		  char device_name[64];
		  err = hsa_agent_get_info(device->agent_id, HSA_AGENT_INFO_NAME, device_name);
		  device->name = device_name;
	  }
#endif
	  POCL_RETURN_DEVICE_INFO_STR(device->name);
	}
	case CL_DEVICE_VENDOR                            :
	{
#if defined HSA_RUNTIME
	  if(device->type&CL_DEVICE_TYPE_GPU){
		  char vendor_name[64];
		  err = hsa_agent_get_info(device->agent_id, HSA_AGENT_INFO_VENDOR_NAME, vendor_name);
		  device->vendor = vendor_name;
	  }
#endif
	  POCL_RETURN_DEVICE_INFO_STR(device->vendor);
	}
	case CL_DRIVER_VERSION                           :   //pocl default
	  POCL_RETURN_DEVICE_INFO_STR(device->driver_version);
	case CL_DEVICE_PROFILE                           ://by clinfo
	  POCL_RETURN_DEVICE_INFO_STR(device->profile);
	case CL_DEVICE_VERSION                           ://pocl default
	  POCL_RETURN_DEVICE_INFO_STR(device->version);
	case CL_DEVICE_EXTENSIONS                        ://TODO: Empty string for now. please fix it.
	  POCL_RETURN_DEVICE_INFO_STR(device->extensions);
	case CL_DEVICE_PLATFORM                          ://pocl default
	  {
		/* Return the first platform id, assuming this is the only
		   platform id (which is currently always the case for pocl) */
		cl_platform_id platform_id;
		POname(clGetPlatformIDs)(1, &platform_id, NULL);
		POCL_RETURN_GETINFO(cl_platform_id, platform_id);
	  }
	case CL_DEVICE_DOUBLE_FP_CONFIG                  : //TODO: by ccchen, Not Support Yet!!
	  POCL_RETURN_DEVICE_INFO_WITH_IMPL_CHECK(cl_ulong, device->double_fp_config);
	case CL_DEVICE_HALF_FP_CONFIG                    : break;//TODO: by ccchen, Not Support Yet!!
	case CL_DEVICE_PREFERRED_VECTOR_WIDTH_HALF       ://TODO: by ccchen, Not Support Yet!!
	  POCL_RETURN_DEVICE_INFO_WITH_IMPL_CHECK(cl_uint, device->preferred_vector_width_half);
	case CL_DEVICE_HOST_UNIFIED_MEMORY               : //TODO: by ccchen, please check this.
	  POCL_RETURN_GETINFO(cl_bool, CL_TRUE);
	case CL_DEVICE_NATIVE_VECTOR_WIDTH_CHAR          :  //TODO: by ccchen, Not Support Yet!!
	  POCL_RETURN_DEVICE_INFO_WITH_IMPL_CHECK(cl_uint, device->native_vector_width_char);
	case CL_DEVICE_NATIVE_VECTOR_WIDTH_SHORT         : //TODO: by ccchen, Not Support Yet!!
	  POCL_RETURN_DEVICE_INFO_WITH_IMPL_CHECK(cl_uint, device->native_vector_width_short);
	case CL_DEVICE_NATIVE_VECTOR_WIDTH_INT           : //TODO: by ccchen, Not Support Yet!!
	  POCL_RETURN_DEVICE_INFO_WITH_IMPL_CHECK(cl_uint, device->native_vector_width_int);
	case CL_DEVICE_NATIVE_VECTOR_WIDTH_LONG          : //TODO: by ccchen, Not Support Yet!!
	  POCL_RETURN_DEVICE_INFO_WITH_IMPL_CHECK(cl_uint, device->native_vector_width_long);
	case CL_DEVICE_NATIVE_VECTOR_WIDTH_FLOAT         : //TODO: by ccchen, Not Support Yet!!
	  POCL_RETURN_DEVICE_INFO_WITH_IMPL_CHECK(cl_uint, device->native_vector_width_float);
	case CL_DEVICE_NATIVE_VECTOR_WIDTH_DOUBLE        : //TODO: by ccchen, Not Support Yet!!
	  POCL_RETURN_DEVICE_INFO_WITH_IMPL_CHECK(cl_uint, device->native_vector_width_double);
	case CL_DEVICE_NATIVE_VECTOR_WIDTH_HALF          : //TODO: by ccchen, Not Support Yet!!
	  POCL_RETURN_DEVICE_INFO_WITH_IMPL_CHECK(cl_uint, device->native_vector_width_half);
	case CL_DEVICE_OPENCL_C_VERSION                  :
	  POCL_RETURN_DEVICE_INFO_STR("OpenCL C 2.0");
	}
  return CL_INVALID_VALUE;
}
POsym(clGetDeviceInfo)
