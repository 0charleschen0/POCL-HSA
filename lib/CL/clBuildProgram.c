/* OpenCL runtime library: clBuildProgram()

   Copyright (c) 2011-2013 Universidad Rey Juan Carlos,
                           Pekka Jääskeläinen / Tampere Univ. of Technology
   
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
#include "install-paths.h"
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

//#define TEST_TIME
#ifdef TEST_TIME
	#include <time.h>
#endif

#define MEM_ASSERT(x, err_jmp) do{ if (x){errcode = CL_OUT_OF_HOST_MEMORY;goto err_jmp;}} while(0)
#define COMMAND_LENGTH 4096

CL_API_ENTRY cl_int CL_API_CALL
POname(clBuildProgram)(cl_program program,
               cl_uint num_devices,
               const cl_device_id *device_list,
               const char *options,
               void (CL_CALLBACK *pfn_notify)(cl_program program, void *user_data),
               void *user_data) CL_API_SUFFIX__VERSION_1_0
{
  char tmpdir[POCL_FILENAME_LENGTH];
  char device_tmpdir[POCL_FILENAME_LENGTH];
  char source_file_name[POCL_FILENAME_LENGTH], binary_file_name[POCL_FILENAME_LENGTH];
  FILE *source_file, *binary_file;
  size_t n;
  struct stat buf;
  char command[COMMAND_LENGTH];
  int errcode;
  int i;
  int error;
  unsigned char *binary;
  unsigned real_num_devices;
  const cl_device_id *real_device_list;
  /* The default build script for .cl files. */
  char *pocl_build_script;
  int device_i = 0;
  const char *user_options = "";

  if (program == NULL)
  {
    errcode = CL_INVALID_PROGRAM;
    goto ERROR;
  }

  if (pfn_notify == NULL && user_data != NULL)
  {
    errcode = CL_INVALID_VALUE;
    goto ERROR;
  }

  if (program->kernels)
  {
    errcode = CL_INVALID_OPERATION;
    goto ERROR;
  }

  if (options != NULL)
    {
      user_options = options;
      program->compiler_options = strdup(options);
    }
  else
    {
      free(program->compiler_options);
      program->compiler_options = NULL;        
    }  

  if (program->source == NULL && program->binaries == NULL)
  {
    errcode = CL_INVALID_PROGRAM;
    goto ERROR;
  }

  if ((num_devices > 0 && device_list == NULL) ||
      (num_devices == 0 && device_list != NULL))
  {
    errcode = CL_INVALID_VALUE;
    goto ERROR;
  }
      
  if (num_devices == 0)
    {
      real_num_devices = program->num_devices;
      real_device_list = program->devices;
    } else
    {
      real_num_devices = num_devices;
      real_device_list = device_list;
    }

  if (program->binaries == NULL)
    {
      snprintf (tmpdir, POCL_FILENAME_LENGTH, "%s/", program->temp_dir);
      mkdir (tmpdir, S_IRWXU);

      if (((program->binary_sizes =
           (size_t *) malloc (sizeof (size_t) * real_num_devices)) == NULL) 
              || (program->binaries = 
           (unsigned char**) calloc( real_num_devices, sizeof (unsigned char*))) == NULL)
      {
        errcode = CL_OUT_OF_HOST_MEMORY;
        goto ERROR_CLEAN_BINARIES;
      }

      snprintf 
        (source_file_name, POCL_FILENAME_LENGTH, "%s/%s", tmpdir, 
         POCL_PROGRAM_CL_FILENAME);

      source_file = fopen(source_file_name, "w+");
      if (source_file == NULL)
      {
        errcode = CL_OUT_OF_HOST_MEMORY;
        goto ERROR_CLEAN_BINARIES;
      }

      n = fwrite (program->source, 1,
                  strlen(program->source), source_file);
      fclose(source_file);

#if defined HSA_RUNTIME
   for(i=0; i<program->num_devices; ++i){
	   if(program->devices[i]->type&CL_DEVICE_TYPE_GPU){
		  hsa_status_t err;
		  char str[65535];
		  char * command;
//compile kernel code to hsail by CLOC
		  if((command = getenv("HSA_OPENCL2BRIG_COMMAND")) != NULL){
			  sprintf(str,"%s %s",command,source_file_name);
		  }else{
             if(program->compiler_options != NULL){
#ifdef DEBUG
			 sprintf(str,"cloc.sh -clopts \"%s\" %s",program->compiler_options,source_file_name);
#else
			 sprintf(str,"cloc.sh -q -clopts \"%s\" %s",program->compiler_options,source_file_name);
#endif
			 }else{
#ifdef DEBUG
			 sprintf(str,"cloc.sh %s",source_file_name);
#else
			 sprintf(str,"cloc.sh -q %s",source_file_name);
#endif
			 }
		  }
#ifdef TEST_TIME
#define BILLION 1000000000L
		  double psr_total_time;
		  struct timespec psr_start, psr_end;
		  clock_gettime(CLOCK_REALTIME, &psr_start);
		  int error = system(str);
		  clock_gettime(CLOCK_REALTIME, &psr_end);
		  psr_total_time = (psr_end.tv_sec - psr_start.tv_sec)+ ((double)(psr_end.tv_nsec-psr_start.tv_nsec))/(double)BILLION;
		  printf("CLOC takes %lf sec to compile.\n", (double)psr_total_time);
#else
		  int error = system(str);
#endif
		  /*
		   * Load BRIG, encapsulated in an ELF container, into a BRIG module.
		   */
		  sprintf(binary_file_name,"%s/program.brig",tmpdir);
		  err = create_brig_module_from_brig_file(binary_file_name, &(program->brigModule));
#ifndef DEBUG
		  if(err != HSA_STATUS_SUCCESS)
#endif
		  check(Creating the brig module from .brig, err);
		  hsa_agent_t device = program->devices[i]->agent_id;
		  /*
		   * Create hsa program.
		   */
		  hsa_ext_program_handle_t hsaProgram;
		  err = hsa_ext_program_create(&device, 1, HSA_EXT_BRIG_MACHINE_LARGE, HSA_EXT_BRIG_PROFILE_FULL, &hsaProgram);
#ifndef DEBUG
	  if(err != HSA_STATUS_SUCCESS)
#endif
		  check(Creating the hsa program, err);
		  program->hsaProgram = hsaProgram;
		  /*
		  * Add the BRIG module to hsa program.
		  */
		  err = hsa_ext_add_module(hsaProgram, program->brigModule, &(program->module));
#ifndef DEBUG
		  if(err != HSA_STATUS_SUCCESS)
#endif
			  check(Adding the brig module to the program, err);
	  }
   }
#endif

      if (n < strlen(program->source))
      {
        errcode = CL_OUT_OF_HOST_MEMORY;
        goto ERROR_CLEAN_BINARIES;
      }


      if (getenv("POCL_BUILDING") != NULL)
        pocl_build_script = BUILDDIR "/scripts/" POCL_BUILD;
      else if (access(PKGDATADIR "/" POCL_BUILD, X_OK) == 0)
        pocl_build_script = PKGDATADIR "/" POCL_BUILD;
      else
        pocl_build_script = POCL_BUILD;

      /* Build the fully linked non-parallel bitcode for all
         devices. */
      for (device_i = 0; device_i < real_num_devices; ++device_i)
        {
    	  cl_device_id device = real_device_list[device_i];
#if defined HSA_RUNTIME
         if(device->type&CL_DEVICE_TYPE_GPU){
        	  FILE* binary_file = fopen(binary_file_name, "r");
			  fseek(binary_file, 0, SEEK_END);
			  if (binary_file == NULL)
			  {
				  errcode = CL_OUT_OF_HOST_MEMORY;
				  goto ERROR_CLEAN_BINARIES;
			  }
			  program->binary_sizes[device_i] = ftell(binary_file);
				fseek(binary_file, 0, SEEK_SET);

				unsigned char* binary = (unsigned char *) malloc(program->binary_sizes[device_i]);
				if (binary == NULL)
				{
					errcode = CL_OUT_OF_HOST_MEMORY;
					goto ERROR_CLEAN_BINARIES;
				}

				n = fread(binary, 1, program->binary_sizes[device_i], binary_file);
				if (n < program->binary_sizes[device_i])
				  {
					  errcode = CL_OUT_OF_HOST_MEMORY;
					  goto ERROR_CLEAN_BINARIES;
				  }
				program->binaries[device_i] = binary;
          }else
#endif
          {
			  program->binaries[device_i] = NULL;
			  snprintf (device_tmpdir, POCL_FILENAME_LENGTH, "%s/%s",
						program->temp_dir, device->name);
			  mkdir (device_tmpdir, S_IRWXU);

			  snprintf
				(binary_file_name, POCL_FILENAME_LENGTH, "%s/%s",
				 device_tmpdir, POCL_PROGRAM_BC_FILENAME);

			  if (real_device_list[device_i]->llvm_target_triplet != NULL)
				{
				  error = snprintf(command, COMMAND_LENGTH,
								   "USER_OPTIONS=\"%s\" %s -t %s -o %s %s",
								   user_options,
								   pocl_build_script,
								   device->llvm_target_triplet,
								   binary_file_name, source_file_name);
				}
			  else
				{
				  error = snprintf(command, COMMAND_LENGTH,
								   "USER_OPTIONS=\"%s\" %s -o %s %s",
								   user_options,
								   pocl_build_script,
								   binary_file_name, source_file_name);
				}

			  if (error < 0)
			  {
				errcode = CL_OUT_OF_HOST_MEMORY;
				goto ERROR_CLEAN_BINARIES;
			  }

			  /* call the customized build command, if needed for the
				 device driver */
			  if (device->build_program != NULL)
				{
				  error = device->build_program
					(device->data, source_file_name, binary_file_name,
					 command, device_tmpdir);
				}
			  else
				{
			  error = system(command);
				}

			  if (error != 0)
			  {
				errcode = CL_BUILD_PROGRAM_FAILURE;
				goto ERROR_CLEAN_BINARIES;
			  }

			  binary_file = fopen(binary_file_name, "r");
			  if (binary_file == NULL)
			  {
				errcode = CL_OUT_OF_HOST_MEMORY;
				goto ERROR_CLEAN_BINARIES;
			  }

			  fseek(binary_file, 0, SEEK_END);

			  program->binary_sizes[device_i] = ftell(binary_file);
			  fseek(binary_file, 0, SEEK_SET);

			  binary = (unsigned char *) malloc(program->binary_sizes[device_i]);
			  if (binary == NULL)
			  {
				  errcode = CL_OUT_OF_HOST_MEMORY;
				  goto ERROR_CLEAN_BINARIES;
			  }

			  n = fread(binary, 1, program->binary_sizes[device_i], binary_file);
			  if (n < program->binary_sizes[device_i])
				{
					errcode = CL_OUT_OF_HOST_MEMORY;
					goto ERROR_CLEAN_BINARIES;
				}
			  program->binaries[device_i] = binary;
			}
        }
    }
  else
    {
      /* Build from a binary. The "binaries" (LLVM bitcodes) are loaded to
         memory in the clBuildWithBinary(). Dump them to the files. */
	  //by ccchen, or .brig file
      for (device_i = 0; device_i < real_num_devices; ++device_i)
        {
          cl_device_id device = real_device_list[device_i];
#ifdef HSA_RUNTIME
//Load program from clCreateProgramFromBinary
          if(device->type&CL_DEVICE_TYPE_GPU){
    		  //by ccchen, copy .brig file to /tmp/poclxxx/program.brig
    		  hsa_status_t err;
    		  snprintf(binary_file_name,POCL_FILENAME_LENGTH, "%s/program.brig" ,program->temp_dir);
    		  FILE* binary_file = fopen(binary_file_name,"w");
    		  fwrite (program->binaries[device_i], 1, program->binary_sizes[device_i],
					  binary_file);
    		  fclose (binary_file);
    		  err = create_brig_module_from_brig_file(binary_file_name, &(program->brigModule));
    		  char command[COMMAND_LENGTH];
//把brig翻成hsail
    		  sprintf(command,"hsailasm -disassemble %s",binary_file_name);
    		  system(command);
#ifndef DEBUG
    		  if(err != HSA_STATUS_SUCCESS)
#endif
    		  check(Creating the brig module from .brig, err);
    		  hsa_agent_t agent = device->agent_id;
    		  /*
    		   * Create hsa program.
    		   */
    		  hsa_ext_program_handle_t hsaProgram;
    		  err = hsa_ext_program_create(&agent, 1, HSA_EXT_BRIG_MACHINE_LARGE, HSA_EXT_BRIG_PROFILE_FULL, &hsaProgram);
#ifndef DEBUG
    		  if(err != HSA_STATUS_SUCCESS)
#endif
    		  check(Creating the hsa program, err);
    		  program->hsaProgram = hsaProgram;
    		  /*
    		  * Add the BRIG module to hsa program.
    		  */
    		  err = hsa_ext_add_module(hsaProgram, program->brigModule, &(program->module));
#ifndef DEBUG
    		  if(err != HSA_STATUS_SUCCESS)
#endif
    			  check(Adding the brig module to the program, err);

    	  }else
#endif
    	  {
    		  int count;
			  count = snprintf (device_tmpdir, POCL_FILENAME_LENGTH, "%s/%s",
						program->temp_dir, real_device_list[device_i]->name);
			  MEM_ASSERT(count >= POCL_FILENAME_LENGTH, ERROR_CLEAN_PROGRAM);

			  error = mkdir (device_tmpdir, S_IRWXU);
			  MEM_ASSERT(error, ERROR_CLEAN_PROGRAM);

			  count = snprintf
				(binary_file_name, POCL_FILENAME_LENGTH, "%s/%s",
				 device_tmpdir, POCL_PROGRAM_BC_FILENAME);
			  MEM_ASSERT(count >= POCL_FILENAME_LENGTH, ERROR_CLEAN_PROGRAM);

			  binary_file = fopen(binary_file_name, "w");
			  MEM_ASSERT(binary_file == NULL, ERROR_CLEAN_PROGRAM);

			  fwrite (program->binaries[device_i], 1, program->binary_sizes[device_i],
					  binary_file);

			  fclose (binary_file);
    	  }
        }      
    }

  return CL_SUCCESS;

  /* Set pointers to NULL during cleanup so that clProgramRelease won't
   * cause a double free. */

ERROR_CLEAN_BINARIES:
  for(i = 0; i < device_i; i++)
  {
    free(program->binaries[i]);
    program->binaries[i] = NULL;
  }
ERROR_CLEAN_PROGRAM:
  free(program->binaries);
  program->binaries = NULL;
  free(program->binary_sizes);
  program->binary_sizes = NULL;
ERROR:
  return errcode;
}
POsym(clBuildProgram)
