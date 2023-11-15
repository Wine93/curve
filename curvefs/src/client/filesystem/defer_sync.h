/*
 *  Copyright (c) 2023 NetEase Inc.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

/*
 * Project: Curve
 * Created Date: 2023-03-06
 * Author: Jingli Chen (Wine93)
 */

#ifndef CURVEFS_SRC_CLIENT_FILESYSTEM_DEFER_SYNC_H_
#define CURVEFS_SRC_CLIENT_FILESYSTEM_DEFER_SYNC_H_

#include <atomic>
#include <vector>
#include <memory>

#include "absl/container/btree_map.h"
#include "src/common/interruptible_sleeper.h"
#include "curvefs/src/client/common/config.h"
#include "curvefs/src/client/filesystem/meta.h"
#include "curvefs/src/client/filesystem/utils.h"

namespace curvefs {
namespace client {
namespace filesystem {

using ::curve::common::RWLock;
using ::curve::common::Mutex;
using ::curve::common::InterruptibleSleeper;
using ::curvefs::client::common::DeferSyncOption;

struct DeferAttr {
    DeferAttr() = default;

    explicit DeferAttr(const InodeAttr& attr) {
        mtime = TimeSpec(attr.mtime(), attr.mtime_ns());
        length = attr.length();
    }

    struct TimeSpec mtime;
    uint64_t length;
};

class DeferInodes {
 public:
    bool Add(const std::shared_ptr<InodeWrapper>& inode);

    bool Get(Ino ino, std::shared_ptr<InodeWrapper>* inode);

    bool Remove(Ino ino);

 private:
    bool ModifiedSince(const std::shared_ptr<InodeWrapper>& now,
                       const std::shared_ptr<InodeWrapper>& old);

 private:
    RWLock rwlock_;
    absl::btree_map<Ino, std::shared_ptr<InodeWrapper>> inodes_;
};

class DeferSync {
 public:
    explicit DeferSync(DeferSyncOption option);

    void Start();

    void Stop();

    void Push(const std::shared_ptr<InodeWrapper>& inode);

    bool IsDefered(Ino ino, std::shared_ptr<InodeWrapper>* inode);

    bool IsDefered(Ino ino, DeferAttr* attr);

 private:
    void DoSync(const std::shared_ptr<InodeWrapper>& inode);

    void SyncTask();

 private:
    DeferSyncOption option_;
    Mutex mutex_;
    std::atomic<bool> running_;
    std::thread thread_;
    InterruptibleSleeper sleeper_;
    std::shared_ptr<DeferInodes> inodes_;
    std::vector<std::shared_ptr<InodeWrapper>> pending_;
};

}  // namespace filesystem
}  // namespace client
}  // namespace curvefs

#endif  // CURVEFS_SRC_CLIENT_FILESYSTEM_DEFER_SYNC_H_
