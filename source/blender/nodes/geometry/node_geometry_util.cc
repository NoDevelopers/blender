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
 */

#include "node_geometry_util.hh"
#include "node_util.h"

#include "DNA_mesh_types.h"
#include "DNA_meshdata_types.h"

#include "BKE_mesh.h"
#include "BKE_mesh_runtime.h"
#include "BKE_pointcloud.h"

namespace blender::nodes {

template<typename Component>
Map<std::string, AttributeInfo> gather_attribute_info(Span<GeometryInstanceGroup> geometry_sets)
{
  Map<std::string, AttributeInfo> attribute_info;
  for (const GeometryInstanceGroup &set_group : geometry_sets) {
    const GeometrySet &set = set_group.geometry_set;
    if (set.has<Component>()) {
      const Component &component = *set.get_component_for_read<Component>();
      for (const std::string &name : component.attribute_names()) {
        ReadAttributePtr attribute = component.attribute_try_get_for_read(name);
        BLI_assert(attribute);
        const CustomDataType data_type = attribute->custom_data_type();
        const AttributeDomain domain = attribute->domain();
        if (attribute_info.contains(name)) {
          AttributeInfo &info = attribute_info.lookup(name);
          info.data_type = attribute_data_type_highest_complexity({info.data_type, data_type});
          info.domain = domain; /* TODO: Use domain priority. */
        }
        else {
          attribute_info.add(name, {data_type});
        }
      }
    }
  }
  return attribute_info;
}

static Mesh *join_mesh_topology_and_builtin_attributes(Span<GeometryInstanceGroup> set_groups)
{
  int totverts = 0;
  int totloops = 0;
  int totedges = 0;
  int totpolys = 0;
  int64_t cd_dirty_vert = 0;
  int64_t cd_dirty_poly = 0;
  int64_t cd_dirty_edge = 0;
  int64_t cd_dirty_loop = 0;
  for (const GeometryInstanceGroup &set_group : set_groups) {
    const GeometrySet &set = set_group.geometry_set;
    if (set.has_mesh()) {
      const Mesh &mesh = *set.get_mesh_for_read();
      totverts += mesh.totvert * set_group.transforms.size();
      totloops += mesh.totloop * set_group.transforms.size();
      totedges += mesh.totedge * set_group.transforms.size();
      totpolys += mesh.totpoly * set_group.transforms.size();
      cd_dirty_vert |= mesh.runtime.cd_dirty_vert;
      cd_dirty_poly |= mesh.runtime.cd_dirty_poly;
      cd_dirty_edge |= mesh.runtime.cd_dirty_edge;
      cd_dirty_loop |= mesh.runtime.cd_dirty_loop;
    }
  }

  Mesh *new_mesh = BKE_mesh_new_nomain(totverts, totedges, 0, totloops, totpolys);
  /* Copy settings from the first input geometry set with a mesh. */
  for (const GeometryInstanceGroup &set_group : set_groups) {
    const GeometrySet &set = set_group.geometry_set;
    if (set.has_mesh()) {
      const Mesh &mesh = *set.get_mesh_for_read();
      BKE_mesh_copy_settings(new_mesh, &mesh);
      break;
    }
  }
  new_mesh->runtime.cd_dirty_vert = cd_dirty_vert;
  new_mesh->runtime.cd_dirty_poly = cd_dirty_poly;
  new_mesh->runtime.cd_dirty_edge = cd_dirty_edge;
  new_mesh->runtime.cd_dirty_loop = cd_dirty_loop;

  int vert_offset = 0;
  int loop_offset = 0;
  int edge_offset = 0;
  int poly_offset = 0;
  for (const GeometryInstanceGroup &set_group : set_groups) {
    const GeometrySet &set = set_group.geometry_set;
    if (set.has_mesh()) {
      const Mesh &mesh = *set.get_mesh_for_read();
      for (const float4x4 &transform : set_group.transforms) {
        for (const int i : IndexRange(mesh.totvert)) {
          const MVert &old_vert = mesh.mvert[i];
          MVert &new_vert = new_mesh->mvert[vert_offset + i];

          new_vert = old_vert;

          const float3 new_position = transform * float3(old_vert.co);
          copy_v3_v3(new_vert.co, new_position);
        }
        for (const int i : IndexRange(mesh.totedge)) {
          const MEdge &old_edge = mesh.medge[i];
          MEdge &new_edge = new_mesh->medge[edge_offset + i];
          new_edge = old_edge;
          new_edge.v1 += vert_offset;
          new_edge.v2 += vert_offset;
        }
        for (const int i : IndexRange(mesh.totloop)) {
          const MLoop &old_loop = mesh.mloop[i];
          MLoop &new_loop = new_mesh->mloop[loop_offset + i];
          new_loop = old_loop;
          new_loop.v += vert_offset;
          new_loop.e += edge_offset;
        }
        for (const int i : IndexRange(mesh.totpoly)) {
          const MPoly &old_poly = mesh.mpoly[i];
          MPoly &new_poly = new_mesh->mpoly[poly_offset + i];
          new_poly = old_poly;
          new_poly.loopstart += loop_offset;
        }

        vert_offset += mesh.totvert;
        loop_offset += mesh.totloop;
        edge_offset += mesh.totedge;
        poly_offset += mesh.totpoly;
      }
    }
  }

  return new_mesh;
}

template<typename Component>
static void join_attributes(Span<GeometryInstanceGroup> set_groups,
                            const Map<std::string, AttributeInfo> &attribute_info,
                            GeometryComponent &result)
{
  for (Map<std::string, AttributeInfo>::Item entry : attribute_info.items()) {
    StringRef name = entry.key;
    const AttributeDomain domain_output = entry.value.domain;
    const CustomDataType data_type_output = entry.value.data_type;
    const CPPType *cpp_type = bke::custom_data_type_to_cpp_type(data_type_output);
    BLI_assert(cpp_type != nullptr);

    result.attribute_try_create(entry.key, domain_output, data_type_output);
    WriteAttributePtr write_attribute = result.attribute_try_get_for_write(name);
    if (!write_attribute || &write_attribute->cpp_type() != cpp_type ||
        write_attribute->domain() != domain_output) {
      continue;
    }
    fn::GMutableSpan dst_span = write_attribute->get_span_for_write_only();

    int offset = 0;
    for (const GeometryInstanceGroup &set_group : set_groups) {
      const GeometrySet &set = set_group.geometry_set;
      if (set.has<Component>()) {
        const GeometryComponent &component = *set.get_component_for_read<Component>();
        const int domain_size = component.attribute_domain_size(domain_output);
        ReadAttributePtr source_attribute = component.attribute_try_get_for_read(
            name, domain_output, data_type_output);

        if (source_attribute) {
          fn::GSpan src_span = source_attribute->get_span();
          const void *src_buffer = src_span.data();
          for (const int UNUSED(i) : set_group.transforms.index_range()) {
            void *dst_buffer = dst_span[offset];
            cpp_type->copy_to_initialized_n(src_buffer, dst_buffer, domain_size);
            offset += domain_size;
          }
        }
        else {
          offset += domain_size * set_group.transforms.size();
        }
      }
    }

    write_attribute->apply_span();
  }
}

static void join_components_mesh(Span<GeometryInstanceGroup> set_groups, GeometrySet &result)
{

  Mesh *new_mesh = join_mesh_topology_and_builtin_attributes(set_groups);

  MeshComponent &dst_component = result.get_component_for_write<MeshComponent>();
  dst_component.replace(new_mesh);

  /* The position attribute is handled above already. */
  Map<std::string, AttributeInfo> attributes = gather_attribute_info<MeshComponent>(set_groups);
  attributes.remove("position");
  join_attributes<MeshComponent>(
      set_groups, attributes, static_cast<GeometryComponent &>(dst_component));
}

static void join_components_pointcloud(Span<GeometryInstanceGroup> set_groups, GeometrySet &result)
{
  int totpoint = 0;
  for (const GeometryInstanceGroup &set_group : set_groups) {
    const GeometrySet &set = set_group.geometry_set;
    if (set.has<PointCloudComponent>()) {
      const PointCloudComponent &component = *set.get_component_for_read<PointCloudComponent>();
      totpoint += component.attribute_domain_size(ATTR_DOMAIN_POINT);
    }
  }

  PointCloudComponent &dst_component = result.get_component_for_write<PointCloudComponent>();
  PointCloud *pointcloud = BKE_pointcloud_new_nomain(totpoint);
  dst_component.replace(pointcloud);

  Map<std::string, AttributeInfo> attributes = gather_attribute_info<PointCloudComponent>(
      set_groups);
  join_attributes<PointCloudComponent>(
      set_groups, attributes, static_cast<GeometryComponent &>(dst_component));
}

static void join_components_volume(Span<GeometryInstanceGroup> set_groups, GeometrySet &result)
{
  /* Not yet supported. Joining volume grids with the same name requires resampling of at least
   * one of the grids. The cell size of the resulting volume has to be determined somehow. */
  VolumeComponent &dst_component = result.get_component_for_write<VolumeComponent>();
  UNUSED_VARS(set_groups, dst_component);
}

void geometry_set_realize_instances_for_write(GeometrySet &geometry_set)
{
  if (!geometry_set.has_instances()) {
    return;
  }

  Vector<GeometryInstanceGroup> set_groups = BKE_geometry_set_gather_instanced(geometry_set);
  join_components_mesh(set_groups, geometry_set);
  join_components_pointcloud(set_groups, geometry_set);
  join_components_volume(set_groups, geometry_set);

  InstancesComponent &instances = geometry_set.get_component_for_write<InstancesComponent>();
  instances.clear();
}

/**
 * Update the availability of a group of input sockets with the same name,
 * used for switching between attribute inputs or single values.
 *
 * \param mode: Controls which socket of the group to make available.
 * \param name_is_available: If false, make all sockets with this name unavailable.
 */
void update_attribute_input_socket_availabilities(bNode &node,
                                                  const StringRef name,
                                                  const GeometryNodeAttributeInputMode mode,
                                                  const bool name_is_available)
{
  const GeometryNodeAttributeInputMode mode_ = (GeometryNodeAttributeInputMode)mode;
  LISTBASE_FOREACH (bNodeSocket *, socket, &node.inputs) {
    if (name == socket->name) {
      const bool socket_is_available =
          name_is_available &&
          ((socket->type == SOCK_STRING && mode_ == GEO_NODE_ATTRIBUTE_INPUT_ATTRIBUTE) ||
           (socket->type == SOCK_FLOAT && mode_ == GEO_NODE_ATTRIBUTE_INPUT_FLOAT) ||
           (socket->type == SOCK_VECTOR && mode_ == GEO_NODE_ATTRIBUTE_INPUT_VECTOR) ||
           (socket->type == SOCK_RGBA && mode_ == GEO_NODE_ATTRIBUTE_INPUT_COLOR));
      nodeSetSocketAvailability(socket, socket_is_available);
    }
  }
}

static int attribute_data_type_complexity(const CustomDataType data_type)
{
  switch (data_type) {
    case CD_PROP_BOOL:
      return 0;
    case CD_PROP_INT32:
      return 1;
    case CD_PROP_FLOAT:
      return 2;
    case CD_PROP_FLOAT2:
      return 3;
    case CD_PROP_FLOAT3:
      return 4;
    case CD_PROP_COLOR:
      return 5;
#if 0 /* These attribute types are not supported yet. */
    case CD_MLOOPCOL:
      return 3;
    case CD_PROP_STRING:
      return 6;
#endif
    default:
      /* Only accept "generic" custom data types used by the attribute system. */
      BLI_assert(false);
      return 0;
  }
}

CustomDataType attribute_data_type_highest_complexity(Span<CustomDataType> data_types)
{
  int highest_complexity = INT_MIN;
  CustomDataType most_complex_type = CD_PROP_COLOR;

  for (const CustomDataType data_type : data_types) {
    const int complexity = attribute_data_type_complexity(data_type);
    if (complexity > highest_complexity) {
      highest_complexity = complexity;
      most_complex_type = data_type;
    }
  }

  return most_complex_type;
}

}  // namespace blender::nodes

bool geo_node_poll_default(bNodeType *UNUSED(ntype), bNodeTree *ntree)
{
  return STREQ(ntree->idname, "GeometryNodeTree");
}

void geo_node_type_base(bNodeType *ntype, int type, const char *name, short nclass, short flag)
{
  node_type_base(ntype, type, name, nclass, flag);
  ntype->poll = geo_node_poll_default;
  ntype->update_internal_links = node_update_internal_links_default;
  ntype->insert_link = node_insert_link_default;
}
