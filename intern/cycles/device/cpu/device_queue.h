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

#pragma once

#include "device/cpu/kernel.h"
#include "device/cpu/kernel_thread_globals.h"
#include "device/device_queue.h"

// clang-format off
#include "kernel/kernel.h"
#include "kernel/kernel_compat_cpu.h"
#include "kernel/kernel_types.h"
// clang-format on

CCL_NAMESPACE_BEGIN

class Device;
class RenderBuffers;

/* Base implementation of all CPU queues. Takes care of kernel function pointers and global data
 * localization. */
class CPUDeviceQueue : public DeviceQueue {
 public:
  CPUDeviceQueue(Device *device, const CPUKernels &kernels, const KernelGlobals &kernel_globals);

 protected:
  CPUKernels kernels_;

  /* Copy of kernel globals which is suitable for concurrent access from multiple queues.
   *
   * More specifically, the `kernel_globals_` is local to this queue and nobody else is
   * accessing it, but some "localization" is required to decouple from kernel globals stored
   * on the device level. */
  CPUKernelThreadGlobals kernel_globals_;
};

class CPUIntegratorQueue : public CPUDeviceQueue {
 public:
  CPUIntegratorQueue(Device *device,
                     const CPUKernels &kernels,
                     const KernelGlobals &kernel_globals,
                     RenderBuffers *render_buffers);

  virtual void enqueue(DeviceKernel kernel) override;

  virtual void set_work_tile(const DeviceWorkTile &work_tile) override;

  virtual bool has_work_remaining() override;

 protected:
  RenderBuffers *render_buffers_;

  /* TODO(sergey): Make integrator state somehow more explicit and more dependent on the number
   * of threads, or number of splits in the kernels.
   * For the quick debug keep it at 1, but it really needs to be changed soon. */
  IntegratorState integrator_state_;

  DeviceWorkTile work_tile_;
};

CCL_NAMESPACE_END
