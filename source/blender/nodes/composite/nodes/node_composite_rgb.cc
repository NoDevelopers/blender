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
 * The Original Code is Copyright (C) 2006 Blender Foundation.
 * All rights reserved.
 */

/** \file
 * \ingroup cmpnodes
 */

#include "BLI_math_vec_types.hh"

#include "DNA_node_types.h"

#include "NOD_compositor_execute.hh"

#include "node_composite_util.hh"

/* **************** RGB ******************** */

namespace blender::nodes::node_composite_rgb_cc {

static void cmp_node_rgb_declare(NodeDeclarationBuilder &b)
{
  b.add_output<decl::Color>(N_("RGBA")).default_value({0.5f, 0.5f, 0.5f, 1.0f});
}

using namespace blender::viewport_compositor;

class RGBOperation : public NodeOperation {
 public:
  using NodeOperation::NodeOperation;

  void execute() override
  {
    Result &result = get_result("RGBA");
    result.allocate_single_value();

    const bNodeSocket *socket = static_cast<bNodeSocket *>(node().outputs.first);
    float4 color = float4(static_cast<bNodeSocketValueRGBA *>(socket->default_value)->value);

    result.set_color_value(color);
  }
};

static NodeOperation *get_compositor_operation(Context &context, DNode node)
{
  return new RGBOperation(context, node);
}

}  // namespace blender::nodes::node_composite_rgb_cc

void register_node_type_cmp_rgb()
{
  namespace file_ns = blender::nodes::node_composite_rgb_cc;

  static bNodeType ntype;

  cmp_node_type_base(&ntype, CMP_NODE_RGB, "RGB", NODE_CLASS_INPUT);
  ntype.declare = file_ns::cmp_node_rgb_declare;
  node_type_size_preset(&ntype, NODE_SIZE_SMALL);
  ntype.get_compositor_operation = file_ns::get_compositor_operation;

  nodeRegisterType(&ntype);
}
