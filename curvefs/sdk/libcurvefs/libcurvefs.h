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

#ifdef __cplusplus

#include <string>
#include <memory>

#include "curvefs/src/client/vfs/vfs.h"
#include "curvefs/src/client/vfs/config.h"
#include "curvefs/sdk/libcurvefs/guid.h"

std::map<std::string, std::vector<CurveFSMount>> activefs;
struct CurveFSMount {
    CurveFSMount(unint32_t uid, std::vector<uint32_t> gids) :
        cfg(Configure::Default()),
        vfs(std::make_shared<VFS>()) {}

    bool checkPermission();
    bool isSuperUser(const std::string user, const std::vector<std::string> groups);
    uint32_t lookupUid(std::string user);
    uint32_t lookupGid(std::string group);
    std::vector<uint32_t> lookupGids(std::string groups);

    uint32_t Uid() { return uid_; }
    uint32_t Gid() { return gid_; }
    std::vector<uint32_t> Gids() { return gids_; }

    std::shared_ptr<Configure> cfg;
    std::shared_ptr<VFS> vfs;

    std::shared_ptr<Mapping> m_;
    std::string user_;
    std::string group_;
    std::string superUser_;
    std::string superGroup_;
    uint32_t uid_;
    uint32_t gid_;
    std::vector<uint32_t> gids_;
    uint16_t umask_;
};

#endif  // __cplusplus

#ifdef __cplusplus
extern "C" {
#endif

uintptr_t curvefs_create();

int curvefs_set_permission(uintptr_t instance_ptr,
                    const char* name,
                    const char* user,
                    const char* grouping);

int curvefs_update_uid_grouping(uintptr_t instance_ptr, 
                                const char* uidStr,
                                const char* grouping);

int curvefs_setowner(uintptr_t instance_ptr,
                    const char* path,
                    const char* owner,
                    const char* group);

// NOTE: instance_ptr is the pointer of CurveFSMount instance.
void conf_set(uintptr_t instance_ptr, const char* key, const char* value);

int curvefs_mount(uintptr_t instance_ptr,
                  const char* fsname,
                  const char* mountpoint);

int curvefs_mkdir(uintptr_t instance_ptr, const char* path, mode_t mode);

int curvefs_rmdir(uintptr_t instance_ptr, const char* path);

#ifdef __cplusplus
}
#endif

#endif  // CURVEFS_SDK_LIBCURVEFS_LIBCURVEFS_H_
