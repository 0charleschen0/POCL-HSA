#include "pocl_cl.h"
#include "assert.h"
#include "pocl_image_util.h"

extern CL_API_ENTRY cl_int CL_API_CALL
POname(clEnqueueReadImage)(cl_command_queue     command_queue,
                   cl_mem               image,
                   cl_bool              blocking_read, 
                   const size_t *       origin, /* [3] */
                   const size_t *       region, /* [3] */
                   size_t               host_row_pitch,
                   size_t               host_slice_pitch, 
                   void *               ptr,
                   cl_uint              num_events_in_wait_list,
                   const cl_event *     event_wait_list,
                   cl_event *           event) 
CL_API_SUFFIX__VERSION_1_0 
{
  cl_int status;
  cl_device_id device = command_queue->device;
  if (event != NULL)
    {
      *event = (cl_event)malloc(sizeof(struct _cl_event));
      if (*event == NULL)
        return CL_OUT_OF_HOST_MEMORY; 
      POCL_INIT_OBJECT(*event);
      (*event)->queue = command_queue;
      (*event)->command_type = CL_COMMAND_READ_IMAGE;
      POCL_UPDATE_EVENT_QUEUED;

    }
  if(blocking_read){
#ifdef HSA_RUNTIME
	  POCL_UPDATE_EVENT_SUBMITTED;
	  POCL_UPDATE_EVENT_RUNNING;
	  if(command_queue->device->type&CL_DEVICE_TYPE_GPU){
		  hsa_ext_image_region_t hsa_image_region;
		  hsa_image_region.image_offset.x = origin[0];
		  hsa_image_region.image_offset.y = origin[1];
		  hsa_image_region.image_offset.z = origin[2];
		  hsa_image_region.image_range.width = region[0];
		  hsa_image_region.image_range.height = region[1];
		  hsa_image_region.image_range.depth = region[2];
		  hsa_status_t err = hsa_ext_image_export
		  ( device->agent_id,
			image->image_handle,
			ptr,
			host_row_pitch,
			host_row_pitch,
			&hsa_image_region,
			NULL
		  );
	#ifndef DEBUG
				if(err != HSA_STATUS_SUCCESS)
	#endif
		  {
			  check(Image export, err);
		  }
		  status = CL_SUCCESS;
	  }else
	#endif
	  {
		  status = pocl_read_image(image,
					   command_queue->device,
					   origin,
					   region,
					   host_row_pitch,
					   host_slice_pitch,
					   ptr);
	  }
	  POCL_UPDATE_EVENT_COMPLETE;
	  if(event != NULL)
		  POname(clReleaseCommandQueue)(command_queue);
  }else
  {
	    _cl_command_node * cmd = malloc(sizeof(_cl_command_node));
	    if (cmd == NULL)
	      return CL_OUT_OF_HOST_MEMORY;

	    cmd->type = CL_COMMAND_READ_BUFFER;
	    cmd->command.read_image.data = device->data;
	    cmd->command.read_image.host_ptr = ptr;
	    cmd->command.read_image.device_ptr = image->device_ptrs[device->dev_id];
	    cmd->command.read_image.image = image;
	    memcpy(cmd->command.read_image.origin, origin, 3*sizeof(size_t));
	    memcpy(cmd->command.read_image.region, region, 3*sizeof(size_t));
	    cmd->command.read_image.row_pitch = host_row_pitch;
	    cmd->command.read_image.slice_pitch = host_slice_pitch;
	    cmd->next = NULL;
	    cmd->event = event ? *event : NULL;
	    POname(clRetainMemObject) (image);
	    LL_APPEND(command_queue->root, cmd);
  }
  return status;
}
POsym(clEnqueueReadImage)
