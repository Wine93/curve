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

#include "curvefs/sdk/libcurvefs/libcurvefs.h"

bool CurveFSMount::checkPermission() {
    return true;
}

bool CurveFSMount::isSuperUser(const std::string user, 
                    const std::vector<std::string> groups) {
    if (user == superUser_) {
		return true
	}
	for (const auto& g : groups) {
		if (g == superGroup_) {
			return true
		}
	}
	return false
}

uint32_t CurveFSMount::lookupUid(std::string user) {
    if (user == superUser_) {
        return 0;
    }
    return m_->lookupUser(user);
}

uint32_t CurveFSMount::lookupGid(std::string group) {
    if (group == superGroup_) {
        return 0;
    }
    return m_->lookupGroup(group);
}

std::vector<uint32_t> CurveFSMount::lookupGids(const std::vector<uint32_t>& groups) {
    std::vector<uint32_t> gids;
    for (auto &group : groups) {
        gids.push_back(lookupGid(group));
    }
    return gids;
}

static CurveFSMount* get_instance(uintptr_t instance_ptr) {
    return reinterpret_cast<CurveFSMount*>(instance_ptr);
}

uintptr_t curvefs_create() {
    auto mount = new CurveFSMount();
    return reinterpret_cast<uintptr_t>(mount);
}

int curvefs_set_permission(uintptr_t instance_ptr, 
                            const char* name,
                            const char* user, 
                            const char* grouping,
                            const char* superuser,
                            const char* supergroup,
                            uint16_t umask) {
    auto mount = get_instance(instance_ptr);
    mount->user_ = user;
    mount->group_ = grouping;
    mount->superUser_ = superuser;
    mount->superGroup_ = supergroup;
    mount->umask_ = umask;

    mount->m_ = Mapping::newMapping(name);
    std::vector<std::string> groups;
    std::stringstream ss(grouping);
    std::string item;
    while (std::getline(ss, item, ',')) {
        groups.push_back(item);
    }

    if mount->isSuperUser(user, groups) {
        mount->uid_ = 0;
        mount->gid_ = 0;
        mount->gids_ = std::vector<uint32_t>{0};
    } else {
        mount->uid_ = mount->m_->lookupUser(user);
        mount->gids_ = mount->m_->lookupGroup(grouping);
        if (!mount->gids_.empty()) {
            mount->gid_ = mount->gids_[0];
        }
    }
    
    return mount->vfs->SetPermission(mount->uid_, mount->gids_, mount->umask_, mount->checkPermission());
}

int curvefs_update_uid_grouping(uintptr_t instance_ptr, 
                            const char* uidStr,
                            const char* grouping) {
    auto mount = get_instance(instance_ptr);

    std::vector<pwent> uids;
    std::istringstream iss(uidStr);
    std::string line;
    if (uidStr != nullptr) {
        while (std::getline(iss, line)) {
            std::vector<std::string> fields;
            std::string token;
            std::istringstream line_iss(line);
            while (std::getline(line_iss, token, ':')) {
                fields.push_back(token);
            }
            if (fields.size() < 2) {
                continue;
            }
            std::string username = fields[0];
            uint32_t uid = std::stoul(fields[1]);
            uids.push_back(pwent{uid, username});
        }
    }
    
    std::vector<pwent> gids;
    std::vector<std::string> groups;
    iss = std::istringstream(grouping);
    if (grouping != nullptr) {
        while (std::getline(iss, line)) {
            std::vector<std::string> fields;
            std::string token;
            std::istringstream line_iss(line);
            while (std::getline(line_iss, token, ':')) {
                fields.push_back(token);
            }
            if (fields.size() < 2) {
                continue;
            }
            std::string gname = fields[0];
            uint32_t gid = std::stoul(fields[1]);
            gids.push_back(pwent{gid, gname});
            if (fields.size() > 2) {
                std::vector<std::string> users = split(fields.back(), ',');
                if (std::find(users.begin(), users.end(), mount->user_) != users.end()) {
                    groups.push_back(gname);
                }
            }
        }
    }
    
    mount->m_->update(uids, gids, false);
    mount->uid_ = mount->lookupUid(mount->user_);
    if (!groups.empty()) {
        mount->gids_ = mount->lookupGids(groups);
    }
    
    return mount->vfs->SetPermission(mount->uid_, mount->gids_, mount->umask_, mount->checkPermission());
}

// FIXME:
int curvefs_setowner(uintptr_t instance_ptr,
                    const char* path,
                    const char* owner,
                    const char* group) {
    auto mount = get_instance(instance_ptr);
    if (owner != nullptr) {
        mount->uid_ = mount->lookupUid(owner);
    }
    if (group != nullptr) {
        mount->gid_ = mount->lookupGid(group);
    }
    return mount->vfs->Chown(path, mount->uid_, mount->gid_);
}

void conf_set(uintptr_t instance_ptr, const char* key, const char* value) {
    auto mount = get_instance(instance_ptr);
    return mount->cfg->Set(key, value);
}

int curvefs_mount(uintptr_t instance_ptr,
                  const char* fsname,
                  const char* mountpoint) {
    auto mount = get_instance(instance_ptr);
    return mount->vfs->Mount(fsname, mountpoint, mount->cfg);
}

int curvefs_mkdir(uintptr_t instance_ptr, const char* path, mode_t mode) {
    auto mount = get_instance(instance_ptr);
    return mount->vfs->MkDir(path, mode);
}

int curvefs_rmdir(uintptr_t instance_ptr, const char* path) {
    auto mount = get_instance(instance_ptr);
    return mount->vfs->RmDir(path, mode);
}

int curvefs_open(uintptr_t instance_ptr, const char* path, int flags, int mode) {
    auto mount = get_instance(instance_ptr);
    return mount->vfs->Open(path, flags, mode);
}

