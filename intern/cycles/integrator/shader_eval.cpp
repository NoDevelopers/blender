/*
 * Copyright 2011-2021 Blender Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "integrator/shader_eval.h"

#include "device/device.h"
#include "device/device_queue.h"

#include "device/cpu/kernel.h"
#include "device/cpu/kernel_thread_globals.h"

#include "util/util_logging.h"
#include "util/util_progress.h"
#include "util/util_tbb.h"

CCL_NAMESPACE_BEGIN

ShaderEval::ShaderEval(Device *device, Progress &progress) : device_(device), progress_(progress)
{
  DCHECK_NE(device_, nullptr);
}

bool ShaderEval::eval(const ShaderEvalType type,
                      device_vector<uint4> &input,
                      device_vector<float4> &output)
{
  /* Allocate and copy device buffers. */
  DCHECK_EQ(input.device, device_);
  DCHECK_EQ(output.device, device_);
  DCHECK_LE(output.size(), input.size());

  output.zero_to_device();
  input.copy_to_device();

  /* Evaluate on CPU or GPU. */
  const bool success = (device_->info.type == DEVICE_CPU) ? eval_cpu(type, input, output) :
                                                            eval_gpu(type, input, output);

  /* Copy data back from device if not cancelled. */
  if (success) {
    output.copy_from_device(0, 1, output.size());
  }

  return success;
}

bool ShaderEval::eval_cpu(const ShaderEvalType type,
                          device_vector<uint4> &input,
                          device_vector<float4> &output)
{
  vector<CPUKernelThreadGlobals> kernel_thread_globals;
  device_->get_cpu_kernel_thread_globals(kernel_thread_globals);

  /* Find required kernel function. */
  const CPUKernels &kernels = *(device_->get_cpu_kernels());

  /* Simple parallel_for over all work items. */
  const int64_t work_size = output.size();
  uint4 *input_data = input.data();
  float4 *output_data = output.data();
  bool success = true;

  tbb::parallel_for(int64_t(0), work_size, [&](int64_t work_index) {
    /* TODO: is this fast enough? */
    if (progress_.get_cancel()) {
      success = false;
      return;
    }

    const int thread_index = tbb::this_task_arena::current_thread_index();
    KernelGlobals *kg = &kernel_thread_globals[thread_index];

    switch (type) {
      case SHADER_EVAL_DISPLACE:
        kernels.shader_eval_displace(kg, input_data, output_data, work_index);
        break;
      case SHADER_EVAL_BACKGROUND:
        kernels.shader_eval_background(kg, input_data, output_data, work_index);
        break;
      default:
        LOG(FATAL) << "Unhandled shader evaluation " << type << ", should never happen.";
        success = false;
        return;
    }
  });

  return success;
}

bool ShaderEval::eval_gpu(const ShaderEvalType type,
                          device_vector<uint4> &input,
                          device_vector<float4> &output)
{
  /* Find required kernel function. */
  DeviceKernel kernel;
  switch (type) {
    case SHADER_EVAL_DISPLACE:
      kernel = DEVICE_KERNEL_SHADER_EVAL_DISPLACE;
      break;
    case SHADER_EVAL_BACKGROUND:
      kernel = DEVICE_KERNEL_SHADER_EVAL_BACKGROUND;
      break;
    default:
      LOG(FATAL) << "Unhandled shader evaluation " << type << ", should never happen.";
      return false;
  };

  /* Create device queue. */
  unique_ptr<DeviceQueue> queue = device_->queue_create();
  queue->init_execution();

  /* Execute work on GPU in chunk, so we can cancel.
   * TODO : query appropriate size from device.*/
  const int chunk_size = 65536;

  const int work_size = output.size();
  void *d_input = (void *)input.device_pointer;
  void *d_output = (void *)output.device_pointer;

  for (int d_offset = 0; d_offset < work_size; d_offset += chunk_size) {
    int d_work_size = min(chunk_size, work_size - d_offset);
    void *args[] = {&d_input, &d_output, &d_offset, &d_work_size};

    queue->enqueue(kernel, d_work_size, args);
    queue->synchronize();

    if (progress_.get_cancel()) {
      return false;
    }
  }

  return true;
}

CCL_NAMESPACE_END
