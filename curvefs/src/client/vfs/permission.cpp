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
 * Created Date: 2023-07-04
 * Author: caoxianfei1
 */

#include "curvefs/src/client/vfs/meta.h"
#include "curvefs/src/client/vfs/permission.h"

namespace curvefs {
namespace client {
namespace vfs {

Permission::Permission(PermissionOption option)
    : option_(option) {}

bool Permission::Check(Ino ino, ModeMask mmask, InodeAttr* attr) {
    if (!option_.needCheck) {
		return true;
	}
    uint8_t mode = accessMode(attr, option_.uid, option_.gids);
    if ((mode & mmask) != mmask){
        return false;
    }
    return true;
}

uint8_t Permission::accessMode(InodeAttr *attr, 
                    uint32_t uid, 
                    const std::vector<uint32_t>& gids) {
    if (uid == 0) {
        return 0x7;
    }
    uint8_t mode = attr->mode;
    if (uid == attr->uid) {
        return (mode >> 6) & 7;
    }
    for (const auto& gid : gids) {
        if (gid == attr->gid) {
            return (mode >> 3) & 7;
        }
    }
    return mode & 7;
}

}  // namespace vfs
}  // namespace client
}  // namespace curvefs
