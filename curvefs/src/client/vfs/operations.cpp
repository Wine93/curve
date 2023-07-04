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
 * Created Date: 2023-07-07
 * Author: Jingli Chen (Wine93)
 */

#include <vector>

#include "curvefs/src/client/vfs/meta.h"
#include "curvefs/src/client/vfs/operations.h"

namespace curvefs {
namespace client {
namespace vfs {

OperationsImpl::OperationsImpl(std::shared_ptr<FuseClient> client,
                               UserPermissionOption userPerm)
    : client_(client),
      userPerm_(userPerm),
      fs_(client->GetFileSystem()) {}

OperationsImpl::OperationsImpl(std::shared_ptr<FuseClient> client,
                               std::shared_ptr<FileSystem> fs,
                               UserPermissionOption userPerm)
    : client_(client),
      fs_(fs),
      userPerm_(userPerm) {}

std::shared_ptr<FuseContext> OperationsImpl::NewFuseContext() {
    uint32_t gid = 0;
    std::vector<uint32_t> gids = userPerm_.gids;
    if (gids.size() > 0) {
        gid = gids[0];  // primary group
    }
    return std::make_shared<FuseContext>(userPerm_.uid, gid, userPerm_.umask);
}

// init
CURVEFS_ERROR OperationsImpl::Umount() {
    client_->FuseOpDestroy(nullptr);
    client_->Fini();
    client_->UnInit();
    return CURVEFS_ERROR::OK;
}

// directory*
CURVEFS_ERROR OperationsImpl::MkDir(Ino parent,
                                    const std::string& name,
                                    uint16_t mode,
                                    EntryOut* entryOut) {
    auto ctx = NewFuseContext();
    auto req = ctx->GetRequest();
    return client_->FuseOpMkDir(req, parent, name.c_str(), mode, entryOut);
}

CURVEFS_ERROR OperationsImpl::RmDir(Ino parent, const std::string& name) {
    auto ctx = NewFuseContext();
    auto req = ctx->GetRequest();
    return client_->FuseOpRmDir(req, parent, name.c_str());
}

CURVEFS_ERROR OperationsImpl::OpenDir(Ino ino, uint64_t* fh) {
    auto ctx = NewFuseContext();
    auto req = ctx->GetRequest();
    auto fi = ctx->GetFileInfo();
    CURVEFS_ERROR rc = client_->FuseOpOpenDir(req, ino, fi);
    if (rc == CURVEFS_ERROR::OK) {
        *fh = fi->fh;
    }
    return rc;
}

// Don't shock why we readdir by filesystem instead of client :)
CURVEFS_ERROR OperationsImpl::ReadDir(Ino ino,
                                      uint64_t fh,
                                      std::shared_ptr<DirEntryList>* entries) {
    auto ctx = NewFuseContext();
    auto fi = ctx->GetFileInfo();
    fi->fh = fh;
    return fs_->ReadDir(ino, fi, entries);
}

CURVEFS_ERROR OperationsImpl::CloseDir(Ino ino) {
    auto ctx = NewFuseContext();
    auto req = ctx->GetRequest();
    auto fi = ctx->GetFileInfo();
    return client_->FuseOpReleaseDir(req, ino, fi);
}

// file*
CURVEFS_ERROR OperationsImpl::Create(Ino parent,
                                     const std::string& name,
                                     uint16_t mode,
                                     EntryOut* entryOut) {
    auto ctx = NewFuseContext();
    auto req = ctx->GetRequest();
    auto fi = ctx->GetFileInfo();
    return client_->FuseOpCreate(req, parent, name.c_str(),
                                 mode, fi, entryOut);
}

CURVEFS_ERROR OperationsImpl::Open(Ino ino, uint32_t flags) {
    FileOut fileOut;
    auto ctx = NewFuseContext();
    auto req = ctx->GetRequest();
    auto fi = ctx->GetFileInfo();
    fi->flags = flags;
    return client_->FuseOpOpen(req, ino, fi, &fileOut);
}

CURVEFS_ERROR OperationsImpl::Read(Ino ino,
                                   uint64_t offset,
                                   char* buffer,
                                   size_t size,
                                   size_t* nread) {
    auto ctx = NewFuseContext();
    auto req = ctx->GetRequest();
    auto fi = ctx->GetFileInfo();
    return client_->FuseOpRead(req, ino, size, offset, fi, buffer, nread);
}

CURVEFS_ERROR OperationsImpl::Write(Ino ino,
                                    uint64_t offset,
                                    char* buffer,
                                    size_t size,
                                    size_t* nwritten) {
    FileOut fileOut;
    auto ctx = NewFuseContext();
    auto req = ctx->GetRequest();
    auto fi = ctx->GetFileInfo();
    auto rc = client_->FuseOpWrite(req, ino, buffer, size,
                                   offset, fi, &fileOut);
    if (rc == CURVEFS_ERROR::OK) {
        *nwritten = fileOut.nwritten;
    } else {
        *nwritten = 0;
    }
    return rc;
}

CURVEFS_ERROR OperationsImpl::Flush(Ino ino) {
    auto ctx = NewFuseContext();
    auto req = ctx->GetRequest();
    auto fi = ctx->GetFileInfo();
    return client_->FuseOpFlush(req, ino, fi);
}

CURVEFS_ERROR OperationsImpl::Close(Ino ino) {
    auto ctx = NewFuseContext();
    auto req = ctx->GetRequest();
    auto fi = ctx->GetFileInfo();
    return client_->FuseOpRelease(req, ino, fi);
}

CURVEFS_ERROR OperationsImpl::Unlink(Ino parent, const std::string& name) {
    auto ctx = NewFuseContext();
    auto req = ctx->GetRequest();
    return client_->FuseOpUnlink(req, parent, name.c_str());
}

// others
CURVEFS_ERROR OperationsImpl::StatFS(Ino ino, struct statvfs* statvfs) {
    auto ctx = NewFuseContext();
    auto req = ctx->GetRequest();
    return client_->FuseOpStatFs(req, ino, statvfs);
}

CURVEFS_ERROR OperationsImpl::Lookup(Ino parent,
                                     const std::string& name,
                                     EntryOut* entryOut) {
    auto ctx = NewFuseContext();
    auto req = ctx->GetRequest();
    auto rc = client_->FuseOpLookup(req, parent, name.c_str(), entryOut);
    if (rc == CURVEFS_ERROR::OK) {
        fs_->SetEntryTimeout(entryOut);
    }
    return rc;
}

CURVEFS_ERROR OperationsImpl::GetAttr(Ino ino, AttrOut* attrOut) {
    auto ctx = NewFuseContext();
    auto req = ctx->GetRequest();
    auto fi = ctx->GetFileInfo();
    auto rc = client_->FuseOpGetAttr(req, ino, fi, attrOut);
    if (rc == CURVEFS_ERROR::OK) {
        fs_->SetAttrTimeout(attrOut);
    }
    return rc;
}

CURVEFS_ERROR OperationsImpl::SetAttr(Ino ino, struct stat* stat, int toSet) {
    AttrOut attrOut;
    auto ctx = NewFuseContext();
    auto req = ctx->GetRequest();
    auto fi = ctx->GetFileInfo();
    return client_->FuseOpSetAttr(req, ino, stat, toSet, fi, &attrOut);
}

CURVEFS_ERROR OperationsImpl::ReadLink(Ino ino, std::string* link) {
    auto ctx = NewFuseContext();
    auto req = ctx->GetRequest();
    return client_->FuseOpReadLink(req, ino, link);
}

CURVEFS_ERROR OperationsImpl::Rename(Ino parent,
                                     const std::string& name,
                                     Ino newparent,
                                     const std::string& newname) {
    auto ctx = NewFuseContext();
    auto req = ctx->GetRequest();
    return client_->FuseOpRename(req, parent, name.c_str(),
                                 newparent, newname.c_str(), 0);
}

// utility
void OperationsImpl::Attr2Stat(InodeAttr* attr, struct stat* stat) {
    return fs_->Attr2Stat(attr, stat);
}

}  // namespace vfs
}  // namespace client
}  // namespace curvefs
