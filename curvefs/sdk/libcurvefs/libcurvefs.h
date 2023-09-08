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
 * Created Date: 2023-07-12
 * Author: Jingli Chen (Wine93)
 */

#ifndef CURVEFS_SDK_LIBCURVEFS_LIBCURVEFS_H_
#define CURVEFS_SDK_LIBCURVEFS_LIBCURVEFS_H_

#include <stdint.h>
#include <sys/stat.h>
#include <sys/types.h>

#ifdef __cplusplus

#include <string>
#include <sstream>
#include <memory>

#include "curvefs/sdk/libcurvefs/guid.h"
#include "curvefs/src/client/vfs/config.h"
#include "curvefs/src/client/vfs/vfs.h"

using ::curvefs::client::vfs::Configure;
using ::curvefs::client::vfs::VFS;

typedef struct {
    // permission
    bool checkPermission();
    bool isSuperUser(const std::string user, const std::vector<std::string> groups);
    uint32_t lookupUid(std::string user);
    uint32_t lookupGid(std::string group);
    std::string uid2name(uint32_t uid);
    std::string gid2name(uint32_t gid);
    std::vector<uint32_t> lookupGids(const std::vector<std::string>& groups);

    uint32_t Uid() { return uid_; }
    uint32_t Gid() { return gid_; }
    std::vector<uint32_t> Gids() { return gids_; }

    std::shared_ptr<Mapping> m_;
    std::string user_;
    std::string group_;
    std::string superUser_;
    std::string superGroup_;
    uint32_t uid_;
    uint32_t gid_;
    std::vector<uint32_t> gids_;
    uint32_t umask_;

    std::shared_ptr<Configure> cfg;
    std::shared_ptr<VFS> vfs;
} curvefs_mount_t;

#endif  // __cplusplus

// Must be synchronized with DirStream if changed
typedef struct {
    uint64_t ino;
    uint64_t fh;
    uint64_t offset;
} dir_stream_t;

typedef struct {
    struct stat stat;
    char name[256];
} dirent_t;

#ifdef __cplusplus
extern "C" {
#endif

uintptr_t curvefs_create();

void curvefs_release(uintptr_t instance_ptr);

// NOTE: instance_ptr is the pointer of curvefs_mount_t instance.
void curvefs_conf_set(uintptr_t instance_ptr,
                      const char* key,
                      const char* value);

int curvefs_mount(uintptr_t instance_ptr,
                  const char* fsname,
                  const char* mountpoint);

int curvefs_set_guids(uintptr_t instance_ptr, 
                            const char* name,
                            const char* user, 
                            const char* grouping,
                            const char* superuser,
                            const char* supergroup,
                            uint16_t umask);

int curvefs_update_guids(uintptr_t instance_ptr, 
                            const char* uidStr,
                            const char* grouping);

int curvefs_umonut(uintptr_t instance_ptr);

// directory
int curvefs_mkdir(uintptr_t instance_ptr, const char* path, uint16_t mode);

int curvefs_mkdirs(uintptr_t instance_ptr, const char* path, uint16_t mode);

int curvefs_rmdir(uintptr_t instance_ptr, const char* path);

int curvefs_opendir(uintptr_t instance_ptr,
                    const char* path,
                    dir_stream_t* dir_stream);

ssize_t curvefs_readdir(uintptr_t instance_ptr,
                        dir_stream_t* dir_stream,
                        dirent_t* dirent);

int curvefs_closedir(uintptr_t instance_ptr, dir_stream_t* dir_stream);

// file
int curvefs_open(uintptr_t instance_ptr,
                 const char* path,
                 uint32_t flags,
                 uint16_t mode);

int curvefs_lseek(uintptr_t instance_ptr,
                  int fd,
                  uint64_t offset,
                  int whence);

ssize_t curvefs_read(uintptr_t instance_ptr,
                     int fd,
                     char* buffer,
                     size_t count);

ssize_t curvefs_write(uintptr_t instance_ptr,
                      int fd,
                      char* buffer,
                      size_t count);

int curvefs_fsync(uintptr_t instance_ptr, int fd);

int curvefs_close(uintptr_t instance_ptr, int fd);

int curvefs_unlink(uintptr_t instance_ptr, const char* path);

// others
int curvefs_statfs(uintptr_t instance_ptr,
                   const char* path,
                   struct statvfs* statvfs);

int curvefs_lstat(uintptr_t instance_ptr, const char* path, struct stat* stat);

int curvefs_fstat(uintptr_t instance_ptr, int fd, struct stat* stat);

int curvefs_setattr(uintptr_t instance_ptr,
                    const char* path,
                    struct stat* stat,
                    int to_set);

int curvefs_chmod(uintptr_t instance_ptr, const char* path, uint16_t mode);

int curvefs_rename(uintptr_t instance_ptr,
                   const char* oldpath,
                   const char* newpath);

int curvefs_setowner(uintptr_t instance_ptr,
                    const char* path,
                    const char* user,
                    const char* group);

std::string curvefs_lookup_owner(uintptr_t instance_ptr, uint32_t uid);

std::string curvefs_lookup_group(uintptr_t instance_ptr, uint32_t gid);

#ifdef __cplusplus
}
#endif

#endif  // CURVEFS_SDK_LIBCURVEFS_LIBCURVEFS_H_
