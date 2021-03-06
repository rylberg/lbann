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
//
// data_reader_triplet .hpp .cpp - data reader to use triplet patches
//                                 generated offline.
////////////////////////////////////////////////////////////////////////////////

#ifndef DATA_READER_TRIPLET_HPP
#define DATA_READER_TRIPLET_HPP

#include "data_reader_multi_images.hpp"
#include "cv_process.hpp"
#include "offline_patches_npz.hpp"
#include <vector>
#include <string>
#include <utility>
#include <iostream>

namespace lbann {
class data_reader_triplet : public data_reader_multi_images {
 public:
  using label_t = offline_patches_npz::label_t;
  using sample_t = offline_patches_npz::sample_t;

  data_reader_triplet(const std::shared_ptr<cv_process>& pp, bool shuffle = true);
  data_reader_triplet(const data_reader_triplet&);
  data_reader_triplet& operator=(const data_reader_triplet&);
  ~data_reader_triplet() override;

  data_reader_triplet* copy() const override {
    return new data_reader_triplet(*this);
  }

  std::string get_type() const override {
    return "data_reader_triplet";
  }

  /** Set up imagenet specific input parameters
   *  If argument is set to 0, then this method does not change the value of
   *  the corresponding parameter. However, width and height can only be both
   *  zero or both non-zero.
   */
  void set_input_params(const int width, const int height, const int num_ch,
                        const int num_labels) override;

  // dataset specific functions
  void load() override;

  /// Return the sample list of current minibatch
  std::vector<sample_t> get_image_list_of_current_mb() const;

  /// Allow read-only access to the entire sample list
  std::vector<sample_t> get_image_list() const;

  sample_t get_sample(size_t idx) const {
    return m_samples.get_sample(idx);
  }

  /// sets up a data_store.
  void setup_data_store(model *m) override;

 protected:
  void set_defaults() override;
  bool fetch_datum(::Mat& X, int data_id, int mb_idx, int tid) override;
  bool fetch_label(::Mat& Y, int data_id, int mb_idx, int tid) override;

 protected:
  offline_patches_npz m_samples;
};

}  // namespace lbann

#endif  // DATA_READER_TRIPLET_HPP
