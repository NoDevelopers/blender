/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * The Original Code is Copyright (C) 2011 Blender Foundation.
 * All rights reserved.
 */

/** \file
 * \ingroup cmpnodes
 */

#include "BLI_math_vector.h"

#include "UI_interface.h"
#include "UI_resources.h"

#include "NOD_compositor_execute.hh"

#include "node_composite_util.hh"

/* **************** Transform  ******************** */

namespace blender::nodes::node_composite_transform_cc {

static void cmp_node_transform_declare(NodeDeclarationBuilder &b)
{
  b.add_input<decl::Color>(N_("Image"))
      .default_value({0.8f, 0.8f, 0.8f, 1.0f})
      .is_compositor_domain_input();
  b.add_input<decl::Float>(N_("X")).default_value(0.0f).min(-10000.0f).max(10000.0f);
  b.add_input<decl::Float>(N_("Y")).default_value(0.0f).min(-10000.0f).max(10000.0f);
  b.add_input<decl::Float>(N_("Angle"))
      .default_value(0.0f)
      .min(-10000.0f)
      .max(10000.0f)
      .subtype(PROP_ANGLE);
  b.add_input<decl::Float>(N_("Scale")).default_value(1.0f).min(0.0001f).max(CMP_SCALE_MAX);
  b.add_output<decl::Color>(N_("Image"));
}

static void node_composit_buts_transform(uiLayout *layout, bContext *UNUSED(C), PointerRNA *ptr)
{
  uiItemR(layout, ptr, "filter_type", UI_ITEM_R_SPLIT_EMPTY_NAME, "", ICON_NONE);
}

using namespace blender::viewport_compositor;

class TransformOperation : public NodeOperation {
 public:
  using NodeOperation::NodeOperation;

  void allocate() override
  {
    const Result &input = get_input("Image");
    Result &result = get_result("Image");
    if (input.is_texture) {
      result.allocate_texture(input.size(), &texture_pool());
    }
    else {
      result.allocate_single_value(&texture_pool());
    }
  }

  void execute() override
  {
    const Result &input = get_input("Image");
    Result &result = get_result("Image");
    if (!result.is_texture) {
      copy_v4_v4(result.value, input.value);
      return;
    }

    GPU_texture_copy(result.texture, input.texture);

    const float2 translation = float2(*get_input("X").value, *get_input("Y").value);
    const float rotation = *get_input("Angle").value;
    const float2 scale = float2(*get_input("Scale").value);

    const Transformation2D transformation = Transformation2D::from_translation_rotation_scale(
        translation, rotation, scale);

    result.transformation = transformation * input.transformation;
  }
};

static NodeOperation *get_compositor_operation(Context &context, DNode node)
{
  return new TransformOperation(context, node);
}

}  // namespace blender::nodes::node_composite_transform_cc

void register_node_type_cmp_transform()
{
  namespace file_ns = blender::nodes::node_composite_transform_cc;

  static bNodeType ntype;

  cmp_node_type_base(&ntype, CMP_NODE_TRANSFORM, "Transform", NODE_CLASS_DISTORT);
  ntype.declare = file_ns::cmp_node_transform_declare;
  ntype.draw_buttons = file_ns::node_composit_buts_transform;
  ntype.get_compositor_operation = file_ns::get_compositor_operation;

  nodeRegisterType(&ntype);
}
