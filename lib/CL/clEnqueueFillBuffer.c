/*
 * clEnqueueFillBuffer.c
 *
 *  Created on: Mar 23, 2015
 *      Author: moea-hsa
 */
#include "pocl_cl.h"
#include "utlist.h"
#include <assert.h>

CL_API_ENTRY cl_int CL_API_CALL
POname(clEnqueueFillBuffer)( cl_command_queue command_queue,
		cl_mem buffer,
		const void *pattern,
		size_t pattern_size,
		size_t offset,
		size_t cb,
		cl_uint num_events_in_wait_list,
		const cl_event *event_wait_list,
		cl_event *event)CL_API_SUFFIX__VERSION_1_2
{
	  cl_device_id device;
	  unsigned i;

	  if (command_queue == NULL)
	    return CL_INVALID_COMMAND_QUEUE;

	  if (buffer == NULL)
	    return CL_INVALID_MEM_OBJECT;

	  if (command_queue->context != buffer->context)
	    return CL_INVALID_CONTEXT;

	  if ((pattern == NULL) ||
	      (offset + cb > buffer->size))
	    return CL_INVALID_VALUE;
	  //int check_patten_size_map[8] = {1, 2, 4, 8, 16, 32, 64, 128};
	  if(((pattern_size - 1)&pattern_size)||pattern_size > 128 || pattern_size < 1)
		  return CL_INVALID_VALUE;
	  if((!offset%pattern_size)||!(cb%pattern_size))
		  return CL_INVALID_VALUE;
	  if (num_events_in_wait_list > 0 && event_wait_list == NULL)
	    return CL_INVALID_EVENT_WAIT_LIST;

	  if (num_events_in_wait_list == 0 && event_wait_list != NULL)
	    return CL_INVALID_EVENT_WAIT_LIST;

	  for(i=0; i<num_events_in_wait_list; i++)
	    if (event_wait_list[i] == NULL)
	      return CL_INVALID_EVENT_WAIT_LIST;

	  device = command_queue->device;

	  for (i = 0; i < command_queue->context->num_devices; ++i)
	    {
	        if (command_queue->context->devices[i] == device)
	            break;
	    }
	  assert(i < command_queue->context->num_devices);

	  if (event != NULL)
	    {
	      *event = (cl_event)malloc(sizeof(struct _cl_event));
	      if (*event == NULL)
	        return CL_OUT_OF_HOST_MEMORY;
	      POCL_INIT_OBJECT(*event);
	      (*event)->queue = command_queue;
	      (*event)->command_type = CL_COMMAND_READ_BUFFER;

	      POname(clRetainCommandQueue) (command_queue);

	      POCL_UPDATE_EVENT_QUEUED;
	    }


	  /* enqueue the read, or execute directly */
	  /* TODO: why do we implement both? direct execution seems
	     unnecessary. */

	  if (command_queue->properties & CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE)
		{
		  /* wait for the event in event_wait_list to finish */
		  POCL_ABORT_UNIMPLEMENTED("Out of order queue is not implemented.");
		}
	  else
		{
		  /* in-order queue - all previously enqueued commands must
		   * finish before this read */
		  // ensure our buffer is not freed yet
		  POname(clRetainMemObject) (buffer);
		  POname(clFinish)(command_queue);
		}
	  /* TODO: offset computation doesn't work in case the ptr is not
		 a direct pointer */
	  POCL_UPDATE_EVENT_SUBMITTED;
	  POCL_UPDATE_EVENT_RUNNING;

	  device->fill(device->data, pattern, pattern_size, buffer->device_ptrs[device->dev_id]+offset, cb);

	  POCL_UPDATE_EVENT_COMPLETE;

	  POname(clReleaseMemObject) (buffer);

	  return CL_SUCCESS;
}
POsym(clEnqueueFillBuffer)

