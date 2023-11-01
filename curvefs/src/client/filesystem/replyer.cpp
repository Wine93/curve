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

#include "curvefs/src/client/filesystem/error.h"
#include "curvefs/src/client/filesystem/replyer.h"

namespace curvefs {
namespace client {
namespace filesystem {

void FuseReplyer::Attr2Stat(InodeAttr* attr, struct stat* stat) {
    std::memset(stat, 0, sizeof(struct stat));
    stat->st_ino = attr->inodeid();  //  inode number
    stat->st_mode = attr->mode();  // permission mode
    stat->st_nlink = attr->nlink();  // number of links
    stat->st_uid = attr->uid();  // user ID of owner
    stat->st_gid = attr->gid();  // group ID of owner
    stat->st_size = attr->length();  // total size, in bytes
    stat->st_rdev = attr->rdev();  // device ID (if special file)
    stat->st_atim.tv_sec = attr->atime();  // time of last access
    stat->st_atim.tv_nsec = attr->atime_ns();
    stat->st_mtim.tv_sec = attr->mtime();  // time of last modification
    stat->st_mtim.tv_nsec = attr->mtime_ns();
    stat->st_ctim.tv_sec = attr->ctime();  // time of last status change
    stat->st_ctim.tv_nsec = attr->ctime_ns();
    stat->st_blksize = option_.blockSize;  // blocksize for file system I/O
    stat->st_blocks = 0;  // number of 512B blocks allocated
    if (IsS3File(*attr)) {
        stat->st_blocks = (attr->length() + 511) / 512;
    }
}

void FuseReplyer::Entry2Param(EntryOut* entryOut,
                             fuse_entry_param* e) {
    std::memset(e, 0, sizeof(fuse_entry_param));
    e->ino = entryOut->attr.inodeid();
    e->generation = 0;
    Attr2Stat(&entryOut->attr, &e->attr);
    e->entry_timeout = entryOut->entryTimeout;
    e->attr_timeout = entryOut->attrTimeout;
}

void FuseReplyer::ReplyError(void* req, CURVEFS_ERROR code) {
    fuse_reply_err(req, SysErr(code));
}

void FuseReplyer::ReplyEntry(void* req, EntryOut* entryOut) {
    fuse_entry_param e;
    Entry2Param(entryOut, &e);
    fuse_reply_entry(static_cast<fuse_req_t>(req), &e);
}

void FuseReplyer::ReplyAttr(void* req, AttrOut* attrOut) {
    struct stat stat;
    Attr2Stat(&attrOut->attr, &stat);
    fuse_reply_attr(req, &stat, attrOut->attrTimeout);
}

void FuseReplyer::ReplyReadlink(Request req, const std::string& link) {
    fuse_reply_readlink(req, link.c_str());
}

void FuseReplyer::ReplyData(void* req, char* buffer, size_t size) {
    struct fuse_bufvec bufvec = FUSE_BUFVEC_INIT(size);
    bufvec.buf[0].mem = buffer;
    fuse_reply_data(req, &bufvec, FUSE_BUF_SPLICE_MOVE);
}

}  // namespace filesystem
}  // namespace client
}  // namespace curvefs
