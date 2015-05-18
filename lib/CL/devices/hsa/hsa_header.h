
/*TODO: by ccchen, just copy "basic" folder "basic.h"*/
#ifndef POCL_HSA_HEADER_H
#define POCL_HSA_HEADER_H

#include "pocl_cl.h"
#include "pocl_icd.h"
#include "config.h"

#ifndef WORDS_BIGENDIAN
#define WORDS_BIGENDIAN 0
#endif

#include "prototypes.inc"
GEN_PROTOTYPES (hsa)
//source : http://www.geeks3d.com/20140127/amd-catalyst-13-30-140108a-with-kaveri-a10-apu-support-opengl-and-opencl-info/
//TODO: Disable image support(native available)
#define POCL_DEVICES_HSA {						\
		  POCL_DEVICE_ICD_DISPATCH     						\
		  POCL_OBJECT_INIT, \
		  CL_DEVICE_TYPE_GPU | CL_DEVICE_TYPE_DEFAULT, /* type */					\
		  4098, /* vendor_id */							\
		  8, /* max_compute_units */						\
		  3, /* max_work_item_dimensions */					\
		  /* This could be SIZE_T_MAX, but setting it to INT_MAX should suffice, */ \
		  /* and may avoid errors in user code that uses int instead of size_t */ \
		  {256, 256, 256}, /* max_work_item_sizes */       \
		  256, /* max_work_group_size */					\
		  8, /* preferred_wg_size_multiple */                                   \
		  POCL_DEVICES_PREFERRED_VECTOR_WIDTH_CHAR  , /* preferred_vector_width_char */ \
		  POCL_DEVICES_PREFERRED_VECTOR_WIDTH_SHORT , /* preferred_vector_width_short */ \
		  POCL_DEVICES_PREFERRED_VECTOR_WIDTH_INT   , /* preferred_vector_width_int */ \
		  POCL_DEVICES_PREFERRED_VECTOR_WIDTH_LONG  , /* preferred_vector_width_long */ \
		  POCL_DEVICES_PREFERRED_VECTOR_WIDTH_FLOAT , /* preferred_vector_width_float */ \
		  POCL_DEVICES_PREFERRED_VECTOR_WIDTH_DOUBLE, /* preferred_vector_width_double */ \
		  POCL_DEVICES_PREFERRED_VECTOR_WIDTH_HALF,   /* preferred_vector_width_half */ \
		  /* TODO: figure out what the difference between preferred and native widths are. */ \
		  POCL_DEVICES_PREFERRED_VECTOR_WIDTH_CHAR  , /* preferred_vector_width_char */ \
		  POCL_DEVICES_PREFERRED_VECTOR_WIDTH_SHORT , /* preferred_vector_width_short */ \
		  POCL_DEVICES_PREFERRED_VECTOR_WIDTH_INT   , /* preferred_vector_width_int */ \
		  POCL_DEVICES_PREFERRED_VECTOR_WIDTH_LONG  , /* preferred_vector_width_long */ \
		  POCL_DEVICES_PREFERRED_VECTOR_WIDTH_FLOAT , /* preferred_vector_width_float */ \
		  POCL_DEVICES_PREFERRED_VECTOR_WIDTH_DOUBLE, /* preferred_vector_width_double */ \
		  POCL_DEVICES_PREFERRED_VECTOR_WIDTH_HALF  , /* preferred_vector_width_half */ \
		  720, /* max_clock_frequency *//*TODO*by ccchen, watch out the value!!*/						\
		  32, /* address_bits *//*TODO*by ccchen, watch out the value!!*/							\
		  552861696, /* max_mem_alloc_size */						\
		  CL_TRUE, /* image_support */	/*TODO: By ccchen, Not yet implement.*/					\
		  128, /* max_read_image_args */						\
		  64, /* max_write_image_args */						\
		  8192, /* image2d_max_width */						\
		  8192, /* image2d_max_height */						\
		  2048, /* image3d_max_width */						\
		  2048, /* image3d_max_height */						\
		  2048, /* image3d_max_depth */						\
		  16, /* max_samplers */							\
		  2146435072, /* max_parameter_size */						\
		  1024, /* mem_base_addr_align */						\
		  128, /* min_data_type_align_size */					\
		  CL_FP_ROUND_TO_NEAREST | CL_FP_INF_NAN, /* single_fp_config *//*TODO: by ccchen, not for sure*/	\
		  CL_FP_ROUND_TO_NEAREST | CL_FP_INF_NAN, /* double_fp_config *//*TODO: by ccchen, not for sure*/	\
		  CL_NONE, /* global_mem_cache_type *//*TODO: by ccchen, not for sure*/					\
		  65536 , /* global_mem_cacheline_size *//*TODO*by ccchen, watch out the value!!*/					\
		  16384 , /* global_mem_cache_size */	/*TODO*by ccchen, watch out the value!!*/				\
		  2146435072 , /* global_mem_size */	/*TODO*by ccchen, watch out the value!!*/					\
		  6553616 , /* max_constant_buffer_size */ /*TODO*by ccchen, watch out the value!!*/					\
		  8, /* max_constant_args */						\
		  CL_LOCAL, /* local_mem_type */					\
		  32768 , /* local_mem_size */	/*TODO*by ccchen, watch out the value!!*/					\
		  CL_FALSE, /* error_correction_support */				\
		  CL_TRUE, /* host_unified_memory */                \
		  1, /* profiling_timer_resolution */					\
		  !(WORDS_BIGENDIAN), /* endian_little */				\
		  CL_TRUE, /* available */						\
		  CL_TRUE, /* compiler_available */					\
		  CL_EXEC_KERNEL, /*execution_capabilities */				\
		  CL_QUEUE_PROFILING_ENABLE, /* queue_properties */			\
		  0x7f73dd0b5670,/* platform */							\
		  "hsa", /* name */							\
		  "pocl", /* vendor */							\
		  PACKAGE_VERSION, /* driver_version */						\
		  "FULL_PROFILE", /* profile */						\
		  "OpenCL 1.2 pocl", /* version */					\
		  "cl_khr_fp64 cl_amd_fp64 cl_khr_local_int32_base_atomics cl_khr_byte_addressable_store", /* extensions */							\
		  /* implementation */							\
		  pocl_hsa_uninit, /* uninit */                                     \
		  pocl_hsa_init, /* init */                                       \
		  pocl_hsa_malloc, /* malloc */					\
		  pocl_hsa_create_sub_buffer, /* create_sub_buffer */                   \
		  pocl_hsa_free, /* free */						\
		  pocl_hsa_read, /* read */						\
		  pocl_hsa_read_rect, /* read_rect */				\
		  pocl_hsa_write, /* write */					\
		  pocl_hsa_write_rect, /* write_rect */				\
		  pocl_hsa_copy, /* copy */						\
		  pocl_hsa_copy_rect, /* copy_rect */				\
		  pocl_hsa_fill,    /*fill*/                    \
		  pocl_hsa_map_mem,                               \
		  NULL, /* unmap_mem is a NOP */                    \
		  pocl_hsa_run, /* run */                         \
		  pocl_hsa_get_timer_value,  /* get_timer_value */    \
		  NULL, /* build_program */ \
		  pocl_hsa_get_supported_image_formats, /*get supported image formats*/\
		  NULL, /* data */                                  \
		  KERNEL_DIR,  /* kernel_lib_target (forced kernel library dir) */  \
		  OCL_KERNEL_TARGET, /* llvm_target_triplet */                         \
		  1,     /* dev_id */                                   \
          0  /*hsa_agent_t*/                                   \
}

#endif /* POCL_HSA_HEADER_H */
