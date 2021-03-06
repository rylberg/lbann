////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2014-2016, Lawrence Livermore National Security, LLC.
// Produced at the Lawrence Livermore National Laboratory.
// Written by the LBANN Research Team (B. Van Essen, et al.) listed in
// the CONTRIBUTORS file. <lbann-dev@llnl.gov>
//
// LLNL-CODE-697807.
// All rights reserved.
//
// This file is part of LBANN: Livermore Big Artificial Neural Network
// Toolkit. For details, see http://software.llnl.gov/LBANN or
// https://github.com/LLNL/LBANN.
//
// Licensed under the Apache License, Version 2.0 (the "Licensee"); you
// may not use this file except in compliance with the License.  You may
// obtain a copy of the License at:
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
// implied. See the License for the specific language governing
// permissions and limitations under the license.
////////////////////////////////////////////////////////////////////////////////

#include "lbann/metrics/metric.hpp"
#include "lbann/models/model.hpp"
#include "lbann/layers/io/target/generic_target_layer.hpp"
#include "lbann/utils/timer.hpp"

namespace lbann {

void metric_statistics::add_value(EvalType total_value, int num_samples) {
  m_sum += total_value;
  m_num_samples += num_samples;
}

EvalType metric_statistics::get_mean() const {
  if (m_num_samples == 0) {
    std::stringstream err;
    err << __FILE__ << " " << __LINE__ << " :: "
        << "attempted to get mean metric value with no statistics";
    throw lbann_exception(err.str());
  }
  return m_sum / m_num_samples;
}

void metric_statistics::reset() {
  m_sum = 0.0;
  m_num_samples = 0;
}

bool metric_statistics::pack_scalars(persist& p) {
  p.write_double(persist_type::validate, "sum", m_sum);
  p.write_uint64(persist_type::validate, "num_samples", m_num_samples);
  return true;
}

bool metric_statistics::unpack_scalars(persist& p, struct packing_header *header) {
  double sum;
  uint64_t num_samples;
  p.read_double(persist_type::validate, "sum", &sum);
  p.read_uint64(persist_type::validate, "num_samples", (uint64_t *) &num_samples);
  m_sum = sum;
  m_num_samples = num_samples;
  if (header != nullptr) {
    header->sum = sum;
    header->num_samples = num_samples;
  }
  return true;
}

void metric_statistics::unpack_header(struct packing_header& header) {
  m_sum = header.sum;
  m_num_samples = header.num_samples;
}

metric::metric(lbann_comm *comm)
  : m_comm(comm),
    m_target_layer(nullptr) {}

void metric::setup(model& m) {

  // Set target layer if needed
  if (m_target_layer == nullptr) {
    const std::vector<Layer*> layers = m.get_layers();
    for (int i = layers.size() - 1; i >= 0; --i) {
      const generic_target_layer *target = dynamic_cast<const generic_target_layer*>(layers[i]);
      if (target != nullptr) {
        m_target_layer = target;
        break;
      }
    }
    if (m_target_layer == nullptr) {
      std::stringstream err;
      err << __FILE__ << " " << __LINE__ << " :: "
          << "could not setup metric with target layer";
      throw lbann_exception(err.str());
    }
  }

}

EvalType metric::evaluate(execution_mode mode,
                          int mini_batch_size) {

  // Check if target layer pointer has been setup
  if (m_target_layer == nullptr) {
    std::stringstream err;
    err << __FILE__ << " " << __LINE__ << " :: "
        << "attempted to evaluate metric without setting a target layer";
    throw lbann_exception(err.str());
  }

  // Evaluate objective function
  double start = get_time();
  const EvalType total_value = evaluate_compute(m_target_layer->get_prediction(),
                                                m_target_layer->get_ground_truth());
  m_evaluate_time += get_time() - start;

  // Record result in statistics and return
  m_statistics[mode].add_value(total_value, mini_batch_size);
  return total_value / mini_batch_size;

}

EvalType metric::get_mean_value(execution_mode mode) const {
  if (m_statistics.count(mode) == 0
      || m_statistics.at(mode).get_num_samples() == 0) {
    std::stringstream err;
    err << __FILE__ << " " << __LINE__ << " :: "
        << "attempted to get mean metric value with no samples for statistics";
    throw lbann_exception(err.str());
  }
  return m_statistics.at(mode).get_mean();
}

int metric::get_statistics_num_samples(execution_mode mode) const {
  if (m_statistics.count(mode) == 0) {
    return 0;
  } else {
    return m_statistics.at(mode).get_num_samples();
  }
}

const generic_target_layer& metric::get_target_layer() const {
  if (m_target_layer == nullptr) {
    std::stringstream err;
    err << __FILE__ << " " << __LINE__ << " :: "
        << "attempted to access target layer before it is set";
    throw lbann_exception(err.str());
  }
  return *m_target_layer;
}

std::vector<Layer*> metric::get_layer_pointers() const {
  return std::vector<Layer*>(1, const_cast<generic_target_layer *>(m_target_layer));
}

void metric::set_layer_pointers(std::vector<Layer*> layers) {
  if (layers.size() != 1) {
    std::stringstream err;
    err << __FILE__ << " " << __LINE__ << " :: "
        << "attempted to set layer pointers with an invalid number of pointers "
        << "(expected 1, found " << layers.size() << ")";
    throw lbann_exception(err.str());
  }
  m_target_layer = dynamic_cast<const generic_target_layer *>(layers[0]);
}

bool metric::save_to_checkpoint_shared(persist& p) {
  // write out fields we need to save for model
  if (p.get_rank() == 0) {
    m_statistics[execution_mode::training].pack_scalars(p);
    m_statistics[execution_mode::validation].pack_scalars(p);
    m_statistics[execution_mode::testing].pack_scalars(p);
  }
  return true;
}

bool metric::load_from_checkpoint_shared(persist& p) {
  struct metric_statistics::packing_header training_header, validation_header, testing_header;
  if (p.get_rank() == 0) {
    m_statistics[execution_mode::training].unpack_scalars(p, &training_header);
    m_statistics[execution_mode::validation].unpack_scalars(p, &validation_header);
    m_statistics[execution_mode::testing].unpack_scalars(p, &testing_header);
  }

  MPI_Bcast(&training_header, sizeof(training_header), MPI_BYTE, 0, MPI_COMM_WORLD);
  MPI_Bcast(&validation_header, sizeof(validation_header), MPI_BYTE, 0, MPI_COMM_WORLD);
  MPI_Bcast(&testing_header, sizeof(testing_header), MPI_BYTE, 0, MPI_COMM_WORLD);

  m_statistics[execution_mode::training].unpack_header(training_header);
  m_statistics[execution_mode::validation].unpack_header(validation_header);
  m_statistics[execution_mode::testing].unpack_header(testing_header);
  return true;
}


}  // namespace lbann
