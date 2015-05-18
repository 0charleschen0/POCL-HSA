#include "pocl_cl.h"
CL_API_ENTRY cl_int CL_API_CALL
POname(clEnqueueWaitForEvents)(cl_command_queue  command_queue,
                       cl_uint           num_events,
                       const cl_event *  event_list) 
					   CL_EXT_SUFFIX__VERSION_1_1_DEPRECATED
{
  POCL_ABORT_UNIMPLEMENTED("clEnqueueWaitForEvents is not implemented.");
  return CL_SUCCESS;
}
POsym(clEnqueueWaitForEvents)
