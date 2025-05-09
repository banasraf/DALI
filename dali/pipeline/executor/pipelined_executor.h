// Copyright (c) 2017-2024, NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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

#ifndef DALI_PIPELINE_EXECUTOR_PIPELINED_EXECUTOR_H_
#define DALI_PIPELINE_EXECUTOR_PIPELINED_EXECUTOR_H_

#include <memory>
#include <string>
#include <vector>

#include "dali/core/common.h"
#include "dali/core/error_handling.h"
#include "dali/pipeline/executor/executor_impl.h"

namespace dali {

/**
 * @brief In addition to the functionality provided by Executor,
 * the PipelinedExecutor enables pipelined execution by queueing
 * the outputs of each stage (that aren't pipeline outputs - these
 * are already queued by the Executor), and increasing the queue
 * depth to 3. Because we have more, and deeper queues, this
 * executor requires more memory than the normal Executor, but can
 * see large performance benefits from pipelining the cpu, mixed,
 * and gpu portions of the graph.
 */
template <typename WorkspacePolicy, typename QueuePolicy>
class DLL_PUBLIC PipelinedExecutorImpl : public Executor<WorkspacePolicy, QueuePolicy> {
 public:
  DLL_PUBLIC inline PipelinedExecutorImpl(int batch_size, int num_thread, int device_id,
                                          size_t bytes_per_sample_hint, bool set_affinity = false,
                                          QueueSizes prefetch_queue_depth = {2, 2})
      : Executor<WorkspacePolicy, QueuePolicy>(batch_size, num_thread, device_id,
                                               bytes_per_sample_hint, set_affinity,
                                               prefetch_queue_depth) {
  }

  DLL_PUBLIC ~PipelinedExecutorImpl() override = default;

  DISABLE_COPY_MOVE_ASSIGN(PipelinedExecutorImpl);

 protected:
  void SetupOutputInfo(OpGraph &graph) override;

  std::vector<int> GetTensorQueueSizes(const OpGraph &graph) override;

  // Note: Pipelining the cpu, mixed, and gpu execution
  // can be viewed as prefetching each stage w.r.t. the
  // other stages. Thus, we need to queue the outputs of
  // each stage to avoid overwriting data that could still
  // be in use. To do this, we find all outputs of the
  // cpu & mixed stages of the pipeline that aren't
  // outputs requested by the user and setup `queue_depth`
  // extra buffers that we will rotate between. Note that
  // we do not worry about CPU outputs of the mixed
  // stage, as these will only be created as outputs
  // requested by the user.

  std::vector<std::vector<TensorNodeId>> stage_outputs_;

  using Executor<WorkspacePolicy, QueuePolicy>::device_id_;
  using Executor<WorkspacePolicy, QueuePolicy>::stage_queue_depths_;

 private:
  /**
   * @see Executor::CalcIterationDataSize
   */
  size_t CalcIterationDataSize() const override;
};

template <typename WorkspacePolicy, typename QueuePolicy>
void PipelinedExecutorImpl<WorkspacePolicy, QueuePolicy>::SetupOutputInfo(OpGraph &graph) {
  DeviceGuard g(device_id_);
  Executor<WorkspacePolicy, QueuePolicy>::SetupOutputInfo(graph);
  constexpr auto stages_count = static_cast<int>(OpType::COUNT);
  stage_outputs_.resize(stages_count);
  for (int stage = 0; stage < stages_count; stage++) {
    stage_outputs_[stage] = graph.GetStageOutputs(static_cast<OpType>(stage));
  }
}

template <typename WorkspacePolicy, typename QueuePolicy>
std::vector<int> PipelinedExecutorImpl<WorkspacePolicy, QueuePolicy>::GetTensorQueueSizes(
    const OpGraph &graph) {
  std::vector<int> result = Executor<WorkspacePolicy, QueuePolicy>::GetTensorQueueSizes(graph);
  for (int stage = 0; stage < static_cast<int>(OpType::COUNT); stage++) {
    for (auto id : stage_outputs_[stage]) {
      result[id] = stage_queue_depths_[static_cast<OpType>(stage)];
    }
  }
  return result;
}

using PipelinedExecutor =
    PipelinedExecutorImpl<AOT_WS_Policy<UniformQueuePolicy>, UniformQueuePolicy>;

template
class DLL_PUBLIC PipelinedExecutorImpl<AOT_WS_Policy<SeparateQueuePolicy>, SeparateQueuePolicy>;

class DLL_PUBLIC SeparatedPipelinedExecutor
: public PipelinedExecutorImpl<AOT_WS_Policy<SeparateQueuePolicy>, SeparateQueuePolicy> {
  using ImplBase = PipelinedExecutorImpl<AOT_WS_Policy<SeparateQueuePolicy>, SeparateQueuePolicy>;
  using ImplBase::ImplBase;
 public:
  int InputFeedCount(std::string_view name) override;
};

}  // namespace dali

#endif  // DALI_PIPELINE_EXECUTOR_PIPELINED_EXECUTOR_H_
