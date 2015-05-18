/* OpenCL runtime library: clCreateContextFromType()

   Copyright (c) 2011 Universidad Rey Juan Carlos
   
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

#include "devices/devices.h"
#include "pocl_cl.h"
#include <stdlib.h>
#include <string.h>

/* in clCreateContext.c */
int context_set_properties(cl_context                    ctx,
                           const cl_context_properties * properties,
                           cl_int *                      errcode_ret);

#if defined HSA_RUNTIME

hsa_agent_t hsaAgent;
static hsa_device_type_t _deviceType=HSA_DEVICE_TYPE_GPU;

static hsa_status_t find_device(hsa_agent_t agent, void *data) {
    if (data == NULL) {
        return HSA_STATUS_ERROR_INVALID_ARGUMENT;
    }
    hsa_device_type_t device_type;
    hsa_status_t stat =
    hsa_agent_get_info(agent, HSA_AGENT_INFO_DEVICE, &device_type);
    if (stat != HSA_STATUS_SUCCESS) {
        return stat;
    }
    if (device_type == _deviceType) {
        *((hsa_agent_t *)data) = agent;
    }
    return HSA_STATUS_SUCCESS;
}
#endif

CL_API_ENTRY cl_context CL_API_CALL
POname(clCreateContextFromType)(const cl_context_properties *properties,
                        cl_device_type device_type,
                        void (*pfn_notify)(const char *, const void *, size_t, void *),
                        void *user_data,
                        cl_int *errcode_ret) CL_API_SUFFIX__VERSION_1_0
{
  int num_devices;
  int i, j;
  int num_properties;
  int errcode;

  /* initialize libtool here, LT will be needed when loading the kernels */     
  lt_dlinit();
  pocl_init_devices(device_type);

  cl_context context = (cl_context) malloc(sizeof(struct _cl_context));
  if (context == NULL)
  {
    errcode = CL_OUT_OF_HOST_MEMORY;
    goto ERROR;
  }

  POCL_INIT_OBJECT(context);
  context->valid = 0;

  num_properties = context_set_properties(context, properties, &errcode);
  if (errcode)
    {
        goto ERROR_CLEAN_CONTEXT_AND_PROPERTIES;
    }

  num_devices = 0;
  for (i = 0; i < pocl_num_devices; ++i) {
    if ((pocl_devices[i].type & device_type) &&
        (pocl_devices[i].available == CL_TRUE))
      ++num_devices;
  }

  if (num_devices == 0)
    {
      if (errcode_ret != NULL) 
        {
          *errcode_ret = (CL_DEVICE_NOT_FOUND); 
        } 
      /* Return a dummy context so icd call to clReleaseContext() still
         works. This fixes AMD SDK OpenCL samples to work (as of 2012-12-05). */

      return context;
    }

  context->num_devices = num_devices;
  context->devices = (cl_device_id *) malloc(num_devices * sizeof(cl_device_id));
  if (context->devices == NULL)
    {
        errcode = CL_OUT_OF_HOST_MEMORY;
        goto ERROR_CLEAN_CONTEXT_AND_PROPERTIES;
    }
  
  j = 0;
  for (i = 0; i < pocl_num_devices; ++i) {
    if ((pocl_devices[i].type & device_type) &&
	(pocl_devices[i].available == CL_TRUE)) {
      context->devices[j] = &pocl_devices[i];
#if defined HSA_RUNTIME
	  hsa_status_t err;
	  if(device_type&(CL_DEVICE_TYPE_GPU|CL_DEVICE_TYPE_DEFAULT)){
		  if(!context->devices[j]->isHSAinit){
			  err = hsa_init();
			  context->devices[j]->isHSAinit = 1;
	#ifndef DEBUG
		  if(err != HSA_STATUS_SUCCESS)
	#endif
			  check(Initializing the hsa runtime, err);
			  if(err != HSA_STATUS_SUCCESS)
				  return CL_INVALID_PLATFORM;
			  err = hsa_iterate_agents(find_device, &hsaAgent);
	#ifndef DEBUG
		  if(err != HSA_STATUS_SUCCESS)
	#endif
			  check(Calling hsa_iterate_agents, err);
			  if(err != HSA_STATUS_SUCCESS)
				  return CL_INVALID_VALUE;
			  err = (hsaAgent == 0) ? HSA_STATUS_ERROR : HSA_STATUS_SUCCESS;
#ifndef DEBUG
			  if(err != HSA_STATUS_SUCCESS)
#endif
				  check(CHECK_HSAing if the GPU hsaAgent is non-zero, err);
			  if(err != HSA_STATUS_SUCCESS)
				  return CL_INVALID_VALUE;
			  context->devices[i]->agent_id = hsaAgent;
		  }
	}
#endif
      POname(clRetainDevice)(&pocl_devices[i]);
      ++j;
    }
  }   

  if (errcode_ret != NULL)
    *errcode_ret = CL_SUCCESS;
  context->valid = 1;
  return context;

ERROR_CLEAN_CONTEXT_AND_PROPERTIES:
  free(context->properties);
ERROR_CLEAN_CONTEXT:
  free(context);
ERROR:
  if(errcode_ret)
  {
    *errcode_ret = errcode;
  }
  return NULL;
}
POsym(clCreateContextFromType)
