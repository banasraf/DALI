// Copyright (c) 2024, NVIDIA CORPORATION & AFFILIATES. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef DALI_OPERATORS_READER_LOADER_DISCOVER_FILES_S3_H_
#define DALI_OPERATORS_READER_LOADER_DISCOVER_FILES_S3_H_

#include <string>
#include <vector>
#include "dali/operators/reader/loader/discover_files.h"

namespace dali {

std::vector<FileLabelEntry> s3_discover_files(const std::string &file_root,
                                              const FileDiscoveryOptions &opts);

}  // namespace dali

#endif  // DALI_OPERATORS_READER_LOADER_DISCOVER_FILES_S3_H_
