/* OpenCL runtime library: clCreateKernel()

   Copyright (c) 2011 Universidad Rey Juan Carlos and
                 2012 Pekka Jääskeläinen / Tampere Univ. of Technology
   
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
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
//#define DEBUG
#define COMMAND_LENGTH 1024
#if defined HSA_RUNTIME
#define OPENCV_SUPPORT
hsa_status_t err;
/*
 * Finds the specified symbols offset in the specified brig_module.
 * If the symbol is found the function returns HSA_STATUS_SUCCESS,
 * otherwise it returns HSA_STATUS_ERROR.
 */
hsa_status_t find_symbol_offset(hsa_ext_brig_module_t* brig_module,
    char* symbol_name,
	cl_uint*  num_args,
    hsa_ext_brig_code_section_offset32_t* offset) {
#ifdef DEBUG
	printf("Symbol Name: %s\n",symbol_name);
#endif
    /*
     * Get the data section
     */
    hsa_ext_brig_section_header_t* data_section_header =
                brig_module->section[HSA_EXT_BRIG_SECTION_DATA];
    /*
     * Get the code section
     */
    hsa_ext_brig_section_header_t* code_section_header =
             brig_module->section[HSA_EXT_BRIG_SECTION_CODE];

    /*
     * First entry into the BRIG code section
     */
    BrigCodeOffset32_t code_offset = code_section_header->header_byte_count;
    BrigBase* code_entry = (BrigBase*) ((char*)code_section_header + code_offset);
    while (code_offset != code_section_header->byte_count) {
        if (code_entry->kind == BRIG_KIND_DIRECTIVE_KERNEL) {
            /*
             * Now find the data in the data section
             */
            BrigDirectiveExecutable* directive_kernel = (BrigDirectiveExecutable*) (code_entry);
            BrigDataOffsetString32_t data_name_offset = directive_kernel->name;
            BrigData* data_entry = (BrigData*)((char*) data_section_header + data_name_offset);
            if (!strncmp(symbol_name, (char*)data_entry->bytes, strlen(symbol_name))){
#ifdef DEBUG
            	printf("Find Symbol Name : %s\n",data_entry->bytes);
#endif
                *offset = code_offset;
                *num_args = directive_kernel->inArgCount;
                return HSA_STATUS_SUCCESS;
            }
        }
        code_offset += code_entry->byteCount;
        code_entry = (BrigBase*) ((char*)code_section_header + code_offset);
    }
    return HSA_STATUS_ERROR;
}

//hsa_status_t find_num_arg(hsa_ext_brig_module_t* brig_module, int* num_arg)
//{
//	/*
//	     * Get the data section
//	     */
//	    hsa_ext_brig_section_header_t* data_section_header =
//	                brig_module->section[HSA_EXT_BRIG_SECTION_DATA];
//	    /*
//	     * Get the code section
//	     */
//	    hsa_ext_brig_section_header_t* code_section_header =
//	             brig_module->section[HSA_EXT_BRIG_SECTION_CODE];
//
//	    /*
//	     * First entry into the BRIG code section
//	     */
//	    BrigCodeOffset32_t code_offset = code_section_header->header_byte_count;
//	    BrigBase* code_entry = (BrigBase*) ((char*)code_section_header + code_offset);
//	    while (code_offset != code_section_header->byte_count) {
//	            if (code_entry->kind == BRIG_KIND_DIRECTIVE_KERNEL) {
//	                /*
//	                 * Now find the data in the data section
//	                 */
//	                BrigDirectiveExecutable* directive_kernel = (BrigDirectiveExecutable*) (code_entry);
//	                BrigDataOffsetString32_t data_name_offset = directive_kernel->name;
//	                BrigData* data_entry = (BrigData*)((char*) data_section_header + data_name_offset);
//	                *num_arg = directive_kernel->inArgCount + directive_kernel->outArgCount;
//	                return HSA_STATUS_SUCCESS;
//	            }
//	            code_offset += code_entry->byteCount;
//	            code_entry = (BrigBase*) ((char*)code_section_header + code_offset);
//	        }
//	        return HSA_STATUS_ERROR;
//}

#endif

CL_API_ENTRY cl_kernel CL_API_CALL
POname(clCreateKernel)(cl_program program,
               const char *kernel_name,
               cl_int *errcode_ret) CL_API_SUFFIX__VERSION_1_0
{
  cl_kernel kernel;
  char tmpdir[POCL_FILENAME_LENGTH];
  char device_tmpdir[POCL_FILENAME_LENGTH];
  char binary_filename[POCL_FILENAME_LENGTH];
  FILE *binary_file;
  size_t n;
  char descriptor_filename[POCL_FILENAME_LENGTH];
  char command[COMMAND_LENGTH];
  int errcode;
  int error;
  lt_dlhandle dlhandle = NULL;
  int i;
  int device_i;
  char* pocl_kernel_fmt;
#ifdef HSA_RUNTIME
  char hsa_kernel_name[128];
#endif
  if (program == NULL || program->num_devices == 0)
  {
    errcode = CL_INVALID_PROGRAM;
    goto ERROR;
  }

  if (program->binaries == NULL || program->binary_sizes == NULL)
  {
    errcode = CL_INVALID_PROGRAM_EXECUTABLE;
    goto ERROR;
  }

  kernel = (cl_kernel) malloc(sizeof(struct _cl_kernel));
  if (kernel == NULL)
  {
    errcode = CL_OUT_OF_HOST_MEMORY;
    goto ERROR;
  }

  POCL_INIT_OBJECT (kernel);

  if (getenv("POCL_BUILDING") != NULL)
    pocl_kernel_fmt = BUILDDIR "/scripts/" POCL_KERNEL " -k %s -t %s -o %s %s";
  else if (access(PKGDATADIR "/" POCL_KERNEL, X_OK) == 0)
    pocl_kernel_fmt = PKGDATADIR "/" POCL_KERNEL " -k %s -t %s -o %s %s";
  else
    pocl_kernel_fmt = POCL_KERNEL " -k %s -t %s -o %s %s";

  for (device_i = 0; device_i < program->num_devices; ++device_i)
    {
      if (device_i > 0)
        POname(clRetainKernel) (kernel);

#if defined HSA_RUNTIME
      if(program->devices[device_i]->type&CL_DEVICE_TYPE_GPU){
    	  hsa_status_t err;
		  /*
		   * Construct finalization request list.
		   */
		  hsa_ext_finalization_request_t finalization_request_list;
		  finalization_request_list.module = program->module;
		  finalization_request_list.program_call_convention = 0;
#ifdef SCWANG_TEST
		  sprintf(hsa_kernel_name,"&%s",kernel_name);
#else
		  sprintf(hsa_kernel_name,"&__OpenCL_%s_kernel",kernel_name);
#endif
#ifdef DEBUG
		  printf("input kernel name: %s\n",hsa_kernel_name);
#endif
		  err = find_symbol_offset(program->brigModule, hsa_kernel_name, &kernel->hsaArgCount, &finalization_request_list.symbol);
#ifndef DEBUG
	  if(err != HSA_STATUS_SUCCESS)
#endif
		  check(Finding the symbol offset for the kernel, err);

		  /*
		   * Finalize the hsa program.
		   */
		  err = hsa_ext_finalize_program(program->hsaProgram, program->devices[device_i]->agent_id, 1, &finalization_request_list, NULL, NULL, 0, NULL, 0);
#ifndef DEBUG
	  if(err != HSA_STATUS_SUCCESS)
#endif
		  check(Finalizing the program, err);

		  /*
		   * Get the hsa code descriptor address.
		   */
		  err = hsa_ext_query_kernel_descriptor_address(program->hsaProgram, program->module, finalization_request_list.symbol, &(kernel->hsaCodeDescriptor));
#ifndef DEBUG
	  if(err != HSA_STATUS_SUCCESS)
#endif
		  check(Querying the kernel descriptor address, err);
      }else
//End of finalize kernel
#endif
      { snprintf (device_tmpdir, POCL_FILENAME_LENGTH, "%s/%s",
					program->temp_dir, program->devices[device_i]->name);

		  /* If there is no device dir for this device, the program was
			 not built for that device in clBuildProgram. This seems to
			 be OK by the standard. */
		  if (access (device_tmpdir, F_OK) != 0) continue;

		  snprintf (tmpdir, POCL_FILENAME_LENGTH, "%s/%s",
					device_tmpdir, kernel_name);
		  mkdir (tmpdir, S_IRWXU);

		  error = snprintf(binary_filename, POCL_FILENAME_LENGTH,
						   "%s/kernel.bc",
						   tmpdir);
		  if (error < 0)
		  {
			errcode = CL_OUT_OF_HOST_MEMORY;
			goto ERROR_CLEAN_KERNEL;
		  }

		  binary_file = fopen(binary_filename, "w+");
		  if (binary_file == NULL)
		  {
			errcode = CL_OUT_OF_HOST_MEMORY;
			goto ERROR_CLEAN_KERNEL;
		  }

		  n = fwrite(program->binaries[device_i], 1,
					 program->binary_sizes[device_i], binary_file);
		  fclose(binary_file);

		  if (n < program->binary_sizes[device_i])
		  {
			errcode = CL_OUT_OF_HOST_MEMORY;
			goto ERROR_CLEAN_KERNEL;
		  }

		  error |= snprintf(descriptor_filename, POCL_FILENAME_LENGTH,
								 "%s/%s/descriptor.so", device_tmpdir, kernel_name);

			error |= snprintf(command, COMMAND_LENGTH,
							 pocl_kernel_fmt,
							 kernel_name,
							 program->devices[device_i]->llvm_target_triplet,
							 descriptor_filename,
							 binary_filename);
			if (error < 0)
			{
			  errcode = CL_OUT_OF_HOST_MEMORY;
			  goto ERROR_CLEAN_KERNEL;
			}

			error = system(command);
			if (error != 0)
			{
			  errcode = CL_INVALID_KERNEL_NAME;
			  goto ERROR_CLEAN_KERNEL;
			}

		  if (dlhandle == NULL)
		  {
			if (access (descriptor_filename, R_OK) != 0)
			  POCL_ABORT("The kernel descriptor.so was not found.\n");

			dlhandle = lt_dlopen(descriptor_filename);
			if (dlhandle == NULL)
			  {
				fprintf(stderr,
						"Error loading the kernel descriptor from %s (lt_dlerror(): %s)\n",
						descriptor_filename, lt_dlerror());
				errcode = CL_OUT_OF_HOST_MEMORY;
				goto ERROR_CLEAN_KERNEL;
			  }
		  }
       }
    }

//TODO: by ccchen, should I maintain the kernel below if using HSA GPU?
  kernel->function_name = strdup(kernel_name);
  kernel->name = strdup(kernel_name);
#if defined HSA_RUNTIME
// Initialize
  if(program->devices[0]->type&CL_DEVICE_TYPE_GPU){
	  kernel->num_locals = 0;
	  kernel->reqd_wg_size = (int*)malloc(3*sizeof(int));
	  for(i=0; i<3; i++)
		  kernel->reqd_wg_size[i] = 0;
	  if(program->source != NULL){
#ifdef DEBUG
		  printf("Entering create kernel by source!!\n");
#endif
		  char *str, *argName;
		  char program_start[65536];// = (char*)malloc((strlen(program->source)-1)*sizeof(char));
		  strcpy(program_start,program->source);
		  str = strstr(program_start, "kernel");
		  str = strstr(str, "void");
		  str = strstr(str, kernel_name);
		  str+=strlen(kernel_name);
		  //In case other function with the same prefix as kernelName
		  while(strncmp(str,"(",1)!=0 && strncmp(str,"\n",1)!=0
				&& strncmp(str,"\t",1) != 0
				&& strncmp(str," ",1) != 0 && str != "\0")
		  {
			str = strstr(str, kernel_name);
			str+=strlen(kernel_name);
		  }
		  //by ccchen, find number of arguments.
		  char arg_start[65536];
		  int num_args = 1;
		  strcpy(arg_start,str);
		  i = 0;
		  char str_arg[65536];
		  memset(str_arg, 0, 100*sizeof(char));
		  while(arg_start[i] != '{'){
			  str_arg[i] = arg_start[i];
			  if(arg_start[i] == ',')
				  num_args++;
			  i++;
		  }
		  str_arg[i++] = '\0';
		  if(!strcmp(str_arg,"()")||!strcmp(str_arg,"(\n)")||!strcmp(str_arg,"( )")){
			  num_args = 0;
		  }
#ifdef DEBUG
		  printf("num_args : %d\n",num_args);
#endif
		  if(num_args != 0){
			  kernel->arg_is_pointer = (int*)malloc(num_args*sizeof(int));
			  kernel->arg_is_local = (int*)malloc(num_args*sizeof(int));
			  kernel->arg_is_image = (int*)malloc(num_args*sizeof(int));
			  kernel->arg_is_sampler = (int*)malloc(num_args*sizeof(int));
			  kernel->arg_is_struct = (int*)malloc(num_args*sizeof(int));
#ifdef DEBUG
			  printf("%s\n",str_arg);
#endif
//抓argument的type
			  for(i=0; i<num_args; i++){
				if(i == 0){ argName = strtok(str_arg, ",");}
				else if(i == num_args - 1){ argName = strtok(NULL,","); argName = strtok(argName,"{");}
				else {argName = strtok(NULL,",");}
#ifdef DEBUG
				printf("%d : %s\n",i, argName);
#endif
#ifdef OPENCV_SUPPORT
				kernel->arg_is_pointer[i]=(strchr(argName,'*')==NULL && strchr(argName,'[')==NULL && strstr(argName,"IMAGE")==NULL)?0:1;
#else
				kernel->arg_is_pointer[i]=(strchr(argName,'*')==NULL && strchr(argName,'[')==NULL)?0:1;
#endif
				kernel->arg_is_local[i]=(strstr(argName,"local")==NULL)?0:1;
				kernel->arg_is_image[i]=(strstr(argName,"image")==NULL)?0:1;
				kernel->arg_is_sampler[i]=(strstr(argName,"sampler_t")==NULL)?0:1;
				kernel->arg_is_struct[i]=(strstr(argName,"struct")==NULL)?0:1;
				/*
				if(strcmp(kernel->name,"icvFindMaximaInLayer")==0)
					kernel->arg_is_pointer[i] = 0;
					kernel->arg_is_local[i] = 0;
					kernel->arg_is_image[i] = 0;
					kernel->arg_is_sampler[i] = 0;
					kernel->arg_is_struct[i] = 0;
			  */
			  }
			  kernel->num_args = num_args;
#ifdef DEBUG
			  for(i=0; i<num_args; i++)
				  printf("%d, ",kernel->arg_is_pointer[i]);
			  printf("\n");
			  for(i=0; i<num_args; i++)
				  printf("%d, ",kernel->arg_is_local[i]);
			  printf("\n");
			  for(i=0; i<num_args; i++)
				  printf("%d, ",kernel->arg_is_image[i]);
			  printf("\n");
			  for(i=0; i<num_args; i++)
				  printf("%d, ",kernel->arg_is_sampler[i]);
			  printf("\n");
			  for(i=0; i<num_args; i++)
				  printf("%d, ",kernel->arg_is_struct[i]);
			  printf("\n");
#endif
		  }
	  }else{
#ifdef DEBUG
		  printf("Entering create kernel by binary!!\n");
#endif

		  char hsail_file_name[POCL_FILENAME_LENGTH];
		  snprintf(hsail_file_name,POCL_FILENAME_LENGTH, "%s/program.hsail", program->temp_dir);
		  FILE* hsail_file = fopen(hsail_file_name,"r");
		  size_t hsail_file_size = 0;
		  while(fgetc(hsail_file) != EOF)
			  hsail_file_size++;
		  fseek(hsail_file, 0, SEEK_SET);
		  char* hsail_str = (char*)malloc(hsail_file_size*sizeof(char));
		  fread(hsail_str, sizeof(char), hsail_file_size, hsail_file);
		  fclose(hsail_file);
		  char *str, *argName;
		  str = strstr(hsail_str, "kernel");
		  sprintf(hsa_kernel_name,"&__OpenCL_%s_kernel",kernel_name);
		  str = strstr(str, hsa_kernel_name);
		  str+=strlen(hsa_kernel_name);
#ifdef DEBUG
		  printf("kernel name : %s\n",kernel_name);
#endif
		  //In case other function with the same prefix as kernelName
		  while(strncmp(str,"(",1)!=0 && strncmp(str,"\n",1)!=0
				&& strncmp(str,"\t",1) != 0
				&& strncmp(str," ",1) != 0 && str != "\0")
		  {
			str = strstr(str, hsa_kernel_name);
			str+=strlen(hsa_kernel_name);
		  }
		  //by ccchen, find number of argument.
		  char* arg_start;
		  int num_args = 1; //Jump to real argument
		  char* temp = (char*)malloc((strlen(str)+1)*sizeof(char));
		  memset(temp, 0, (strlen(str)+1)*sizeof(char));
//去掉CLOC compiler多加的argument
		  if(strstr(str, "aqlwrap_pointer,") != NULL){
			  str = strstr(str, "aqlwrap_pointer,");
#ifdef DEBUG
			  printf("Binary for HSAIL_HLC_Stable\n");
#endif
			  str += strlen("aqlwrap_pointer,");
		  }
		  free(temp);
		  //snprintf(arg_start,65535,"%s",str);
		  arg_start = (char*)malloc(sizeof(char)*strlen(str));
		  strncpy(arg_start,str , strlen(str));

#if 0
		  printf("arg_start:\n%s",arg_start);
#endif
		  i = 0;
		  char str_arg[512];
		  memset(str_arg, 0, 512*sizeof(char));
		  while(arg_start[i] != '{'){
#ifdef DEBUG
			  printf("%c",arg_start[i]);
#endif
			  str_arg[i] = arg_start[i];
			  if(arg_start[i] == ',')
				  num_args++;
			  i++;
		  }
		  str_arg[i++] = '\0';
#ifdef DEBUG
		  printf("num_args : %d\n",num_args);
#endif
		  kernel->arg_is_pointer = (int*)malloc(num_args*sizeof(int));
		  kernel->arg_is_local = (int*)malloc(num_args*sizeof(int));
		  kernel->arg_is_image = (int*)malloc(num_args*sizeof(int));
		  kernel->arg_is_sampler = (int*)malloc(num_args*sizeof(int));
		  kernel->arg_is_struct = (int*)malloc(num_args*sizeof(int));
#ifdef DEBUG
		  printf("%s\n",str_arg);
#endif
//clCreateProgramWithBinary 只能抓到pointer和scalar
		  for(i=0; i<num_args; i++){
			if(i == 0){ argName = strtok(str_arg, ",");}
			else if(i == num_args - 1){ argName = strtok(NULL,","); argName = strtok(argName,"{");}
			else {argName = strtok(NULL,",");}
#ifdef DEBUG
			printf("%d : %s\n",i, argName);
#endif
			kernel->arg_is_pointer[i]=(strstr(argName,"kernarg_u64")==NULL)?0:1;
			kernel->arg_is_local[i] = kernel->arg_is_image[i] = kernel->arg_is_image[i] = kernel->arg_is_struct[i] = 0;
		  }
		  kernel->num_args = num_args;
		  free(hsail_str);
		  free(arg_start);
#ifdef DEBUG
		  for(i=0; i<num_args; i++)
			  printf("%d, ",kernel->arg_is_pointer[i]);
		  printf("\n");
		  //TODO: By ccchen, could not support image or sampler type.
		  /*
		  for(i=0; i<num_args; i++)
			  printf("%d, ",kernel->arg_is_image[i]);
		  printf("\n");
		  for(i=0; i<num_args; i++)
			  printf("%d, ",kernel->arg_is_sampler[i]);
		  printf("\n");
		  */
#endif
	  }
	}
	  else
#endif
    {
	  kernel->num_args = *(cl_uint *) lt_dlsym(dlhandle, "_num_args");
	  kernel->reqd_wg_size = (int*)lt_dlsym(dlhandle, "_reqd_wg_size");
	  kernel->arg_is_pointer = lt_dlsym(dlhandle, "_arg_is_pointer");
	  kernel->arg_is_local = lt_dlsym(dlhandle, "_arg_is_local");
	  kernel->arg_is_image = lt_dlsym(dlhandle, "_arg_is_image");
	  kernel->arg_is_sampler = lt_dlsym(dlhandle, "_arg_is_sampler");
	  kernel->num_locals = *(cl_uint *) lt_dlsym(dlhandle, "_num_locals");
    }
  kernel->context = program->context;
  kernel->program = program;
  kernel->dlhandle = dlhandle; /* TODO: why is this stored? */

  /* Temporary store for the arguments that are set with clSetKernelArg. */
  kernel->dyn_arguments =
    (struct pocl_argument *) malloc ((kernel->num_args + kernel->num_locals) *
                                     sizeof (struct pocl_argument));
  kernel->next = NULL;

  /* Initialize kernel "dynamic" arguments (in case the user doesn't). */
  for (i = 0; i < kernel->num_args; ++i)
    {
      kernel->dyn_arguments[i].value = NULL;
      kernel->dyn_arguments[i].size = 0;
    }

  /* Fill up automatic local arguments. */
  for (i = 0; i < kernel->num_locals; ++i)
    {
      kernel->dyn_arguments[kernel->num_args + i].value = NULL;
      kernel->dyn_arguments[kernel->num_args + i].size =
        ((unsigned *) lt_dlsym(dlhandle, "_local_sizes"))[i];
    }

  cl_kernel k = program->kernels;
  program->kernels = kernel;
  kernel->next = k;

  POCL_RETAIN_OBJECT(program);

  if (errcode_ret != NULL)
    *errcode_ret = CL_SUCCESS;
  return kernel;

ERROR_CLEAN_KERNEL_AND_CONTENTS:
  free(kernel->function_name);
  free(kernel->name);
  free(kernel->dyn_arguments);
ERROR_CLEAN_KERNEL:
  free(kernel);
ERROR:
  if(errcode_ret != NULL)
  {
    *errcode_ret = errcode;
  }
  return NULL;
}
POsym(clCreateKernel)
