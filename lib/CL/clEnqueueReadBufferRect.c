/* OpenCL runtime library: clEnqueueReadBufferRect()

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

#include "pocl_cl.h"
#include <assert.h>
#include <stdio.h>

CL_API_ENTRY cl_int CL_API_CALL
POname(clEnqueueReadBufferRect)(cl_command_queue command_queue,
                        cl_mem buffer,
                        cl_bool blocking_read,
                        const size_t *buffer_origin,
                        const size_t *host_origin,
                        const size_t *region,
                        size_t buffer_row_pitch,
                        size_t buffer_slice_pitch,
                        size_t host_row_pitch,
                        size_t host_slice_pitch,
                        void *ptr,
                        cl_uint num_events_in_wait_list,
                        const cl_event *event_wait_list,
                        cl_event *event) CL_API_SUFFIX__VERSION_1_1
{
  cl_device_id device;
  unsigned i;

  if (command_queue == NULL)
    return CL_INVALID_COMMAND_QUEUE;

  if (buffer == NULL)
    return CL_INVALID_MEM_OBJECT;

  if (command_queue->context != buffer->context)
    return CL_INVALID_CONTEXT;

  if ((ptr == NULL) ||
      (buffer_origin == NULL) ||
      (host_origin == NULL) ||
      (region == NULL))
    return CL_INVALID_VALUE;

  if ((region[0]*region[1]*region[2] > 0) &&
      (buffer_origin[0] + region[0]-1 +
       buffer_row_pitch * (buffer_origin[1] + region[1]-1) +
       buffer_slice_pitch * (buffer_origin[2] + region[2]-1) >= buffer->size))
    return CL_INVALID_VALUE;

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


  /* execute directly */
  /* TODO: enqueue the read_rect if this is a non-blocking read (see
     clEnqueueReadBuffer) */
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
  POCL_UPDATE_EVENT_SUBMITTED;
  POCL_UPDATE_EVENT_RUNNING;

  /* TODO: offset computation doesn't work in case the ptr is not 
     a direct pointer */
  device->read_rect(device->data, ptr, 
                    buffer->device_ptrs[device->dev_id],
                    buffer_origin, host_origin, region,
                    buffer_row_pitch, buffer_slice_pitch,
                    host_row_pitch, host_slice_pitch);

  POCL_UPDATE_EVENT_COMPLETE;

  POname(clReleaseMemObject) (buffer);

  return CL_SUCCESS;
}
POsym(clEnqueueReadBufferRect)
