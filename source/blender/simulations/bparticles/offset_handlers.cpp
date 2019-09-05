#include "offset_handlers.hpp"

namespace BParticles {

void CreateTrailHandler::execute(OffsetHandlerInterface &interface)
{
  auto positions = interface.attributes().get<float3>("Position");
  auto position_offsets = interface.attribute_offsets().get<float3>("Position");

  auto inputs = m_compute_inputs->compute(interface);

  Vector<float3> new_positions;
  Vector<float> new_birth_times;
  for (uint pindex : interface.pindices()) {
    float rate = inputs->get<float>("Rate", 0, pindex);
    if (rate <= 0.0f) {
      continue;
    }

    TimeSpan time_span = interface.time_span(pindex);

    float factor_start, factor_step;
    time_span.uniform_sample_range(rate, factor_start, factor_step);

    float3 total_offset = position_offsets[pindex] * interface.time_factors()[pindex];
    for (float factor = factor_start; factor < 1.0f; factor += factor_step) {
      float time = time_span.interpolate(factor);
      new_positions.append(positions[pindex] + total_offset * factor);
      new_birth_times.append(time);
    }
  }

  for (std::string &type_name : m_types_to_emit) {
    auto new_particles = interface.particle_allocator().request(type_name, new_positions.size());
    new_particles.set<float3>("Position", new_positions);
    new_particles.set<float>("Birth Time", new_birth_times);

    m_on_birth_action->execute_for_new_particles(new_particles, interface);
  }
}

void SizeOverTimeHandler::execute(OffsetHandlerInterface &interface)
{
  auto birth_times = interface.attributes().get<float>("Birth Time");
  auto sizes = interface.attributes().get<float>("Size");

  auto inputs = m_compute_inputs->compute(interface);

  for (uint pindex : interface.pindices()) {
    float final_size = inputs->get<float>("Final Size", 0, pindex);
    float final_age = inputs->get<float>("Final Age", 1, pindex);

    TimeSpan time_span = interface.time_span(pindex);
    float age = time_span.start() - birth_times[pindex];
    float time_until_end = final_age - age;
    if (time_until_end <= 0.0f) {
      continue;
    }

    float factor = std::min(time_span.duration() / time_until_end, 1.0f);
    float new_size = final_size * factor + sizes[pindex] * (1.0f - factor);
    sizes[pindex] = new_size;
  }
}

}  // namespace BParticles
