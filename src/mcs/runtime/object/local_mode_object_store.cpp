//
// Created by root on 1/12/23.
//

#include "local_mode_object_store.h"

#include <mcs/api/mcs_exception.h>

#include <algorithm>
#include <chrono>
#include <list>
#include <thread>

#include "mcs/runtime/abstract_mcs_runtime.h"

namespace mcs {
    namespace internal {
        LocalModeObjectStore::LocalModeObjectStore(LocalModeMcsRuntime &local_mode_mcs_tuntime)
                : local_mode_mcs_tuntime_(local_mode_mcs_tuntime) {
          memory_store_ = std::make_unique<CoreWorkerMemoryStore>();
        }

        void LocalModeObjectStore::PutRaw(std::shared_ptr<msgpack::sbuffer> data,
                                          ObjectID *object_id) {
            *object_id = ObjectID::FromRandom();
            PutRaw(data, *object_id);
        }

        void LocalModeObjectStore::PutRaw(std::shared_ptr<msgpack::sbuffer> data,
                                          const ObjectID &object_id) {
            auto buffer = std::make_shared<::mcs::LocalMemoryBuffer>(
                    reinterpret_cast<uint8_t *>(data->data()), data->size(), true);
            auto status = memory_store_->Put(
                    ::mcs::McsObject(buffer, nullptr, std::vector<rpc::ObjectReference>()), object_id);
            if (!status) {
                throw McsException("Put object error");
            }
        }

        std::shared_ptr<msgpack::sbuffer> LocalModeObjectStore::GetRaw(const ObjectID &object_id,
                                                                       int timeout_ms) {
            std::vector<ObjectID> object_ids;
            object_ids.push_back(object_id);
            auto buffers = GetRaw(object_ids, timeout_ms);
            MCS_CHECK(buffers.size() == 1);
            return buffers[0];
        }

        std::vector<std::shared_ptr<msgpack::sbuffer>> LocalModeObjectStore::GetRaw(
                const std::vector<ObjectID> &ids, int timeout_ms) {
            std::vector<std::shared_ptr<::mcs::McsObject>> results;
            ::mcs::Status status = memory_store_->Get(ids,
                                                      (int)ids.size(),
                                                      timeout_ms,
                                                      local_mode_mcs_tuntime_.GetWorkerContext(),
                                                      false,
                                                      &results);
            if (!status.ok()) {
                throw McsException("Get object error: " + status.ToString());
            }
            MCS_CHECK(results.size() == ids.size());
            std::vector<std::shared_ptr<msgpack::sbuffer>> result_sbuffers;
            result_sbuffers.reserve(results.size());
            for (size_t i = 0; i < results.size(); i++) {
                auto data_buffer = results[i]->GetData();
                auto sbuffer = std::make_shared<msgpack::sbuffer>(data_buffer->Size());
                sbuffer->write(reinterpret_cast<const char *>(data_buffer->Data()),
                               data_buffer->Size());
                result_sbuffers.push_back(sbuffer);
            }
            return result_sbuffers;
        }

//        std::vector<bool> LocalModeObjectStore::Wait(const std::vector<ObjectID> &ids,
//                                                     int num_objects,
//                                                     int timeout_ms) {
//            absl::flat_hash_set<ObjectID> memory_object_ids;
//            for (const auto &object_id : ids) {
//                memory_object_ids.insert(object_id);
//            }
//            absl::flat_hash_set<ObjectID> ready;
//            ::mcs::Status status = memory_store_->Wait(memory_object_ids,
//                                                       num_objects,
//                                                       timeout_ms,
//                                                       local_mode_mcs_tuntime_.GetWorkerContext(),
//                                                       &ready);
//            if (!status.ok()) {
//                throw McsException("Wait object error: " + status.ToString());
//            }
//            std::vector<bool> result;
//            result.reserve(ids.size());
//            for (size_t i = 0; i < ids.size(); i++) {
//                if (ready.find(ids[i]) != ready.end()) {
//                    result.push_back(true);
//                } else {
//                    result.push_back(false);
//                }
//            }
//            return result;
//        }

        void LocalModeObjectStore::AddLocalReference(const std::string &id) { return; }

        void LocalModeObjectStore::RemoveLocalReference(const std::string &id) { return; }
    }  // namespace internal
}  // namespace mcs