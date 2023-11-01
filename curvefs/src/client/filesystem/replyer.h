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
 * Created Date: 2023-11-01
 * Author: Jingli Chen (Wine93)
 */

#ifndef CURVEFS_SRC_CLIENT_FILESYSTEM_REPLYER_H_
#define CURVEFS_SRC_CLIENT_FILESYSTEM_REPLYER_H_

#include <string>

#include "curvefs/src/client/filesystem/meta.h"

namespace curvefs {
namespace client {
namespace filesystem {

class Replyer {
 public:
    virtual void ReplyError(void* req, CURVEFS_ERROR code) = 0;

    virtual void ReplyEntry(void* req, EntryOut* entryOut) = 0;

    virtual void ReplyAttr(void* req, AttrOut* attrOut) = 0;

    virtual void ReplyReadlink(void* req, const std::string& link) = 0;

    virtual void ReplyData(void* req, const char* buffer, size_t size) = 0;

    virtual void ReplyBuffer(void* req, const char *buf, size_t size) = 0;

    virtual void ReplyOpen(void* req, FileOut* fileOut) = 0;

    virtual void ReplyOpenDir(void* req, FileInfo* fi) = 0;

    virtual ReplyXattr(void* req, size_t size) = 0;

    virtual AddDirEntry(void* req, size_t size) = 0;

    virtual AddDirEntryPlus(void* req, size_t size) = 0;
};

class FuseReplyer : public Replyer {
 public:
    void ReplyError() override;

    void ReplyEntry() override;

    void ReplyAttr() override;

    void ReplyData(void* req, char* buffer, size_t size) override;

 private:
    void Attr2Stat(InodeAttr* attr, struct stat* stat);

    Entry2Param(EntryOut* entryOut, fuse_entry_param* e);
};

}  // namespace filesystem
}  // namespace client
}  // namespace curvefs

#endif  // CURVEFS_SRC_CLIENT_FILESYSTEM_REPLYER_H_
