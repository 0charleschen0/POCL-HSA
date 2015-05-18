/* OpenCL runtime library: clGetDeviceIDs()

   Copyright (c) 2011 Kalle Raiskila 
   
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
#include "devices/devices.h"
#include <string.h>

#if defined HSA_RUNTIME

hsa_agent_t hsaAgent;
static hsa_device_type_t _deviceType=HSA_DEVICE_TYPE_GPU;
hsa_status_t err;

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

CL_API_ENTRY cl_int CL_API_CALL
POname(clGetDeviceIDs)(cl_platform_id   platform, 
               cl_device_type   device_type, 
               cl_uint          num_entries, 
               cl_device_id *   devices, 
               cl_uint *        num_devices) CL_API_SUFFIX__VERSION_1_0
{
  int total_num = 0;
  int devices_added = 0;
  int i;
  pocl_init_devices(device_type);
  /* TODO: OpenCL API specification allows implementation dependent
     behaviour if platform == NULL. Should we just allow it? */
  if (platform == NULL)
    return CL_INVALID_PLATFORM;

  if (num_entries == 0 && devices != NULL)
    return CL_INVALID_VALUE;
  if (num_devices == NULL && devices == NULL)
    return CL_INVALID_VALUE;

  for (i = 0; i < pocl_num_devices; ++i) {

	 if ((pocl_devices[i].type & device_type) &&
        (pocl_devices[i].available == CL_TRUE))
      {
        if (devices != NULL && devices_added < num_entries) 
          {
            devices[devices_added] = &pocl_devices[i];
#if defined HSA_RUNTIME
		    if(devices[devices_added]->type==(CL_DEVICE_TYPE_GPU|CL_DEVICE_TYPE_DEFAULT)){
		    	if(!devices[devices_added]->isHSAinit){
					hsa_status_t err;
					err = hsa_init();
					devices[devices_added]->isHSAinit = 1;
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
					devices[devices_added]->agent_id = hsaAgent;
		    	}
		    }
#endif
            ++devices_added;
          }
        ++total_num;
      }
  }
  
  if (num_devices != NULL)
    *num_devices = total_num;

  if (devices_added > 0 || num_entries == 0)
    return CL_SUCCESS;
  else
    return CL_DEVICE_NOT_FOUND;
}
POsym(clGetDeviceIDs)
