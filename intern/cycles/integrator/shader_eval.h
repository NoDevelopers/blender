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

#include "device/device_memory.h"

#include "kernel/kernel_types.h"

CCL_NAMESPACE_BEGIN

class Device;
class Progress;

/* ShaderEval class performs shader evaluation for background light and displacement. */
class ShaderEval {
 public:
  explicit ShaderEval(Device *device, Progress &progress);

  bool eval(const ShaderEvalType type, device_vector<uint4> &input, device_vector<float4> &output);

 protected:
  bool eval_cpu(const ShaderEvalType type,
                device_vector<uint4> &input,
                device_vector<float4> &output);
  bool eval_gpu(const ShaderEvalType type,
                device_vector<uint4> &input,
                device_vector<float4> &output);

  Device *device_;
  Progress &progress_;
};

CCL_NAMESPACE_END
