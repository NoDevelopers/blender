/*
 * Copyright 2011-2013 Blender Foundation
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

#include "kernel/kernel_write_passes.h"

CCL_NAMESPACE_BEGIN

ccl_device_inline bool svm_node_aov_check(const int path_flag, ccl_global float *render_buffer)
{
  bool is_primary = (path_flag & PATH_RAY_CAMERA) && (!(path_flag & PATH_RAY_SINGLE_PASS_DONE));

  return ((render_buffer != NULL) && is_primary);
}

ccl_device void svm_node_aov_color(INTEGRATOR_STATE_CONST_ARGS,
                                   ShaderData *sd,
                                   float *stack,
                                   uint4 node,
                                   ccl_global float *render_buffer)
{
  float3 val = stack_load_float3(stack, node.y);

  if (render_buffer && !INTEGRATOR_STATE_IS_NULL) {
    const uint32_t render_pixel_index = INTEGRATOR_STATE(path, render_pixel_index);
    const uint64_t render_buffer_offset = (uint64_t)render_pixel_index *
                                          kernel_data.film.pass_stride;
    ccl_global float *buffer = render_buffer + render_buffer_offset +
                               (kernel_data.film.pass_aov_color + node.z);
    kernel_write_pass_float3(buffer, make_float3(val.x, val.y, val.z));
  }
}

ccl_device void svm_node_aov_value(INTEGRATOR_STATE_CONST_ARGS,
                                   ShaderData *sd,
                                   float *stack,
                                   uint4 node,
                                   ccl_global float *render_buffer)
{
  float val = stack_load_float(stack, node.y);

  if (render_buffer && !INTEGRATOR_STATE_IS_NULL) {
    const uint32_t render_pixel_index = INTEGRATOR_STATE(path, render_pixel_index);
    const uint64_t render_buffer_offset = (uint64_t)render_pixel_index *
                                          kernel_data.film.pass_stride;
    ccl_global float *buffer = render_buffer + render_buffer_offset +
                               (kernel_data.film.pass_aov_value + node.z);
    kernel_write_pass_float(buffer, val);
  }
}
CCL_NAMESPACE_END
