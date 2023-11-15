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

#include <vector>
#include <memory>

#include "curvefs/src/client/filesystem/defer_sync.h"
#include "curvefs/src/client/filesystem/utils.h"

namespace curvefs {
namespace client {
namespace filesystem {

using ::curve::common::LockGuard;
using ::curve::common::ReadLockGuard;
using ::curve::common::WriteLockGuard;

bool DeferInodes::ModifiedSince(const std::shared_ptr<InodeWrapper>& now,
                                const std::shared_ptr<InodeWrapper>& old) {
    return true;
}

bool DeferInodes::Add(const std::shared_ptr<InodeWrapper>& inode) {
    WriteLockGuard lk(rwlock_);
    auto ret = inodes_.emplace(inode->GetInodeId(), inode);
    auto iter = ret.first;
    bool yes = ret.second;
    if (yes) {  // not found, insert success
        return true;
    } else if (ModifiedSince(inode, iter->second)) {  // already exist
        iter->second = inode;
        return true;
    }
    return false;
}

bool DeferInodes::Get(Ino ino, std::shared_ptr<InodeWrapper>* inode) {
    ReadLockGuard lk(rwlock_);
    auto iter = inodes_.find(ino);
    if (iter == inodes_.end()) {
        return false;
    }
    *inode = iter->second;
    return true;
}

bool DeferInodes::Remove(Ino ino) {
    WriteLockGuard lk(rwlock_);
    auto iter = inodes_.find(ino);
    if (iter == inodes_.end()) {
        return false;
    }
    inodes_.erase(iter);
    return true;
}

DeferSync::DeferSync(DeferSyncOption option)
    : option_(option),
      mutex_(),
      running_(false),
      thread_(),
      sleeper_(),
      inodes_(std::make_shared<DeferInodes>()),
      pending_() {}

void DeferSync::Start() {
    if (!running_.exchange(true)) {
        thread_ = std::thread(&DeferSync::SyncTask, this);
        LOG(INFO) << "Defer sync thread start success";
    }
}

void DeferSync::Stop() {
    if (running_.exchange(false)) {
        LOG(INFO) << "Stop defer sync thread...";
        sleeper_.interrupt();
        thread_.join();
        LOG(INFO) << "Defer sync thread stopped";
    }
}

void DeferSync::DoSync(const std::shared_ptr<InodeWrapper>& inode) {
    UniqueLock lk(inode->GetUniqueLock());
    inode->Async(nullptr, true);
}

void DeferSync::SyncTask() {
    std::vector<std::shared_ptr<InodeWrapper>> syncing;
    for ( ;; ) {
        bool running = sleeper_.wait_for(std::chrono::seconds(option_.delay));

        {
            LockGuard lk(mutex_);
            syncing.swap(pending_);
        }
        for (const auto& inode : syncing) {
            DoSync(inode);
            inodes_->Remove(inode->GetInodeId());
            LOG(ERROR) << "<<<< ino = " << inode->GetInodeId() << " has synced";
        }
        syncing.clear();

        if (!running) {
            break;
        }
    }
}

void DeferSync::Push(const std::shared_ptr<InodeWrapper>& inode) {
    inodes_->Add(inode);
    LockGuard lk(mutex_);
    pending_.emplace_back(inode);
}

// xxxx: only for nocto scenario
bool DeferSync::IsDefered(Ino ino, std::shared_ptr<InodeWrapper>* inode) {
    return inodes_->Get(ino, inode);
}

bool DeferSync::IsDefered(Ino ino, DeferAttr* attr) {
    std::shared_ptr<InodeWrapper> inode;
    bool yes = inodes_->Get(ino, &inode);
    if (yes) {
        return false;
    }

    InodeAttr out;
    inode->GetInodeAttr(&out);
    *attr = DeferAttr(out);
    return true;
}

}  // namespace filesystem
}  // namespace client
}  // namespace curvefs
