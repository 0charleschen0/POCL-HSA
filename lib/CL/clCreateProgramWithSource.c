/* OpenCL runtime library: clCreateProgramWithSource()

   Copyright (c) 2011 Universidad Rey Juan Carlos and
                 2012 Pekka Jääskeläinen / Tampere University of Technology
   
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
#include <stdlib.h>
#include <string.h>

CL_API_ENTRY cl_program CL_API_CALL
POname(clCreateProgramWithSource)(cl_context context,
                          cl_uint count,
                          const char **strings,
                          const size_t *lengths,
                          cl_int *errcode_ret) CL_API_SUFFIX__VERSION_1_0
{
  cl_program program;
  unsigned size;
  char *source;
  unsigned i;
  int errcode;

  if (count == 0)
  {
    errcode = CL_INVALID_VALUE;
    goto ERROR;
  }

  program = (cl_program) malloc(sizeof(struct _cl_program));
  if (program == NULL)
  {
    errcode = CL_OUT_OF_HOST_MEMORY;
    goto ERROR;
  }

  POCL_INIT_OBJECT(program);

  size = 0;
  for (i = 0; i < count; ++i)
    {
      if (strings[i] == NULL)
      {
        errcode = CL_INVALID_VALUE;
        goto ERROR_CLEAN_PROGRAM;
      }

      if (lengths == NULL)
        size += strlen(strings[i]);
      else if (lengths[i] == 0)
        size += strlen(strings[i]);
      else
        size += lengths[i];
    }
  
  source = (char *) malloc(size + 1);
  if (source == NULL)
  {
    errcode = CL_OUT_OF_HOST_MEMORY;
    goto ERROR_CLEAN_PROGRAM;
  }

  program->source = source;
  program->compiler_options = NULL;

  for (i = 0; i < count; ++i)
    {
      if (lengths == NULL)
        {
          memcpy(source, strings[i], strlen(strings[i]));
          source += strlen(strings[i]);
        }
      else if (lengths[i] == 0)
        {
          memcpy(source, strings[i], strlen(strings[i]));
          source += strlen(strings[i]);
        }
      else
        {
          memcpy(source, strings[i], lengths[i]);
          source += lengths[i];
        }
    }

  *source = '\0';
  char* comment_free_source = (char*)malloc(size + 1);
  char source_copy[65536];
  strcpy(source_copy, program->source);
  i = 0;
  int status = 0, j = 0;
  //Remove comment by FSM
  while(source_copy[i] != '\0'){
  #if 0
  			  printf("%c %d",source_copy[i],status);
  			  //printf("%c",source_copy[i]);
  #endif
	  switch(status){
		  case 0:{//status 0 : Read character
			  if(source_copy[i] == '/')
				  status = 1;
			  else{
				  comment_free_source[j] = source_copy[i];
				  j++;
			  }
		  }break;
		  case 1:{//status 1 : comment mode
			  if(source_copy[i] == '/')
				  status = 2;
			  else if(source_copy[i] == '*')
				  status = 3;
			  else {//regard as devision sign
				  status = 0;
				  comment_free_source[j++] = '/';
				  comment_free_source[j] = source_copy[i];
				  j++;
			  }
		  }break;
		  case 2:{//status 2 : single-line comment mode
			  if(source_copy[i] == '\n'){
				  comment_free_source[j++] = '\n';
				  status = 0;
			  }
		  }break;
		  case 3:{//status 3 : multi-line comment mode
			  if(source_copy[i] == '*')
				  status = 4;
		  }break;
		  case 4:{//status 3 : multi-line comment mode -> wait for '/' out of "*/"
			  if(source_copy[i] == '/')
				  status = 0;
			  else if(source_copy[i] == '*')
				  status = 4;
			  else
				  status = 3;
		  }break;
	  }
	  i++;
  }
  comment_free_source[j++] = '\0';
  program->source = comment_free_source;
#if 0
  printf("%s\n",program->source);
#endif
  program->context = context;
  program->num_devices = context->num_devices;
  program->devices = context->devices;
  program->binary_sizes = NULL;
  program->binaries = NULL;
  program->kernels = NULL;

  /* Create the temporary directory where all kernel files and compilation
     (intermediate) results are stored. */
  program->temp_dir = pocl_create_temp_dir();

  POCL_RETAIN_OBJECT(context);

  if (errcode_ret != NULL)
    *errcode_ret = CL_SUCCESS;
  return program;

ERROR_CLEAN_PROGRAM:
  free(program);
ERROR:
  if(errcode_ret)
  {
    *errcode_ret = errcode;
  }
  return NULL;
}
POsym(clCreateProgramWithSource)
