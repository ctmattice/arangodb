////////////////////////////////////////////////////////////////////////////////
/// DISCLAIMER
///
/// Copyright 2021-2022 ArangoDB GmbH, Cologne, Germany
///
/// Licensed under the Apache License, Version 2.0 (the "License");
/// you may not use this file except in compliance with the License.
/// You may obtain a copy of the License at
///
///     http://www.apache.org/licenses/LICENSE-2.0
///
/// Unless required by applicable law or agreed to in writing, software
/// distributed under the License is distributed on an "AS IS" BASIS,
/// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
/// See the License for the specific language governing permissions and
/// limitations under the License.
///
/// Copyright holder is ArangoDB GmbH, Cologne, Germany
///
/// @author Lars Maier
////////////////////////////////////////////////////////////////////////////////
#include "Logger/LogMacros.h"
#include "UpdateReplicatedState.h"
#include "Replication2/ReplicatedState/StateCommon.h"
#include "Basics/voc-errors.h"
#include "Logger/LogContextKeys.h"

using namespace arangodb::replication2;
using namespace arangodb::replication2::replicated_state;

auto algorithms::updateReplicatedState(
    StateActionContext& ctx, std::string const& serverId, LogId id,
    replicated_state::agency::Plan const* spec,
    replicated_state::agency::Current const* current) -> arangodb::Result {
  if (spec == nullptr) {
    return ctx.dropReplicatedState(id);
  }

  TRI_ASSERT(id == spec->id);
  TRI_ASSERT(spec->participants.contains(serverId));
  auto expectedGeneration = spec->participants.at(serverId).generation;

  auto logCtx =
      LoggerContext(Logger::REPLICATED_STATE).with<logContextKeyLogId>(id);

  LOG_CTX("b089c", TRACE, logCtx) << "Update replicated log" << id
                                  << " for generation " << expectedGeneration;

  auto state = ctx.getReplicatedStateById(id);
  if (state == nullptr) {
    // TODO use user data instead of non-slice
    auto result =
        ctx.createReplicatedState(id, spec->properties.implementation.type);
    if (result.fail()) {
      return result.result();
    }

    state = result.get();

    auto token = std::invoke([&] {
      if (current) {
        if (auto const p = current->participants.find(serverId);
            p != std::end(current->participants)) {
          if (p->second.generation == expectedGeneration) {
            LOG_CTX("19d00", DEBUG, logCtx)
                << "Using existing snapshot information from current";
            // we are allowed to use the information stored here
            return std::make_unique<ReplicatedStateToken>(
                ReplicatedStateToken::withExplicitSnapshotStatus(
                    expectedGeneration, p->second.snapshot));
          } else {
            LOG_CTX("6d8c9", DEBUG, logCtx)
                << "Must not use existing information, generation is "
                   "different. "
                << "Plan = " << expectedGeneration
                << " but Current = " << p->second.generation;
          }
        } else {
          LOG_CTX("cef1a", DEBUG, logCtx)
              << "No snapshot information available for this server "
              << serverId;
        }
      } else {
        LOG_CTX("d4fd8", DEBUG, logCtx)
            << "no current available to read snapshot information from. "
               "Assuming no snapshot available";
      }

      return std::make_unique<ReplicatedStateToken>(expectedGeneration);
    });
    // now start the replicated state
    state->start(std::move(token), spec->properties.implementation.parameters);
    return {TRI_ERROR_NO_ERROR};
  } else {
    auto status = state->getStatus();
    if (status.has_value()) {
      auto generation = status.value().getGeneration();
      if (generation != expectedGeneration) {
        state->flush(expectedGeneration);
      }
    }
    return {TRI_ERROR_NO_ERROR};
  }
}
