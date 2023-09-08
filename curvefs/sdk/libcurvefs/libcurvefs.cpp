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

#include "curvefs/src/client/filesystem/error.h"
#include "curvefs/sdk/libcurvefs/libcurvefs.h"

using ::curvefs::client::filesystem::CURVEFS_ERROR;
using ::curvefs::client::filesystem::SysErr;
using ::curvefs::client::vfs::DirStream;
using ::curvefs::client::vfs::DirEntry;

static curvefs_mount_t* get_instance(uintptr_t instance_ptr) {
    return reinterpret_cast<curvefs_mount_t*>(instance_ptr);
}

bool curvefs_mount_t::checkPermission() {
    return true;
}

bool curvefs_mount_t::isSuperUser(const std::string user, 
                    const std::vector<std::string> groups) {
    if (user == superUser_) {
		return true;
	}
	for (const auto& g : groups) {
		if (g == superGroup_) {
			return true;
		}
	}
	return false;
}

uint32_t curvefs_mount_t::lookupUid(std::string user) {
    if (user == superUser_) {
        return 0;
    }
    return m_->lookupUser(user);
}

uint32_t curvefs_mount_t::lookupGid(std::string group) {
    if (group == superGroup_) {
        return 0;
    }
    return m_->lookupGroup(group);
}

std::string curvefs_mount_t::uid2name(uint32_t uid) {
    std::string name = superUser_;
    if (uid > 0) {
        name = m_->lookupUserID(uid); 
    }
    return name;
}

std::string curvefs_mount_t::gid2name(uint32_t gid) {
    std::string name = superGroup_;
    if (gid > 0) {
        name = m_->lookupGroupID(gid);
    }
    return name;
}

std::vector<uint32_t> curvefs_mount_t::lookupGids(const std::vector<std::string>& groups) {
    std::vector<uint32_t> gids;
    for (auto &group : groups) {
        gids.push_back(lookupGid(group));
    }
    return gids;
}

int curvefs_set_guids(uintptr_t instance_ptr, 
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

    mount->m_ = std::make_shared<Mapping>(Mapping::newMapping(name));
    std::vector<std::string> groups;
    std::stringstream ss(grouping);
    std::string item;
    while (std::getline(ss, item, ',')) {
        groups.push_back(item);
    }

    if (mount->isSuperUser(user, groups)) {
        mount->uid_ = 0;
        mount->gid_ = 0;
        mount->gids_ = std::vector<uint32_t>{0};
    } else {
        mount->uid_ = mount->lookupUid(user);
        mount->gids_ = mount->lookupGids(groups);
        if (!mount->gids_.empty()) {
            mount->gid_ = mount->gids_[0];
        }
    }
    auto rc = mount->vfs->SetPermission(mount->uid_, mount->gids_, mount->umask_, mount->checkPermission());
    return SysErr(rc);
}

int curvefs_update_guids(uintptr_t instance_ptr, 
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
                std::string user;
                std::istringstream user_iss(fields.back());
                while (std::getline(user_iss, user, ',')) {
                    if (user == mount->user_) {
                        groups.push_back(gname);
                    }
                }
            }
        }
    }
    
    mount->m_->update(uids, gids, false);
    mount->uid_ = mount->lookupUid(mount->user_);
    if (!groups.empty()) {
        mount->gids_ = mount->lookupGids(groups);
    }
    auto rc = mount->vfs->SetPermission(mount->uid_, mount->gids_, mount->umask_, mount->checkPermission());
    return SysErr(rc);
}

int curvefs_setowner(uintptr_t instance_ptr,
                    const char* path,
                    const char* user,
                    const char* group) {
    auto mount = get_instance(instance_ptr);
    struct stat stat;
    auto rc = mount->vfs->LStat(path, &stat);
    if (rc != CURVEFS_ERROR::OK) {
        return SysErr(rc);
    }
    uint32_t uid = stat.st_uid;
    uint32_t gid = stat.st_gid;

    if (user != nullptr) {
        uid = mount->lookupUid(user);
    }
    if (group != nullptr) {
        gid = mount->lookupGid(group);
    }
    rc = mount->vfs->Chown(path, uid, gid);
    return SysErr(rc);
}

uintptr_t curvefs_create() {
    auto mount = new curvefs_mount_t();
    mount->cfg = Configure::Default();
    mount->vfs = std::make_shared<VFS>();
    return reinterpret_cast<uintptr_t>(mount);
}

void curvefs_release(uintptr_t instance_ptr) {
    auto mount = get_instance(instance_ptr);
    delete mount;
    mount = nullptr;
}

void curvefs_conf_set(uintptr_t instance_ptr,
                      const char* key,
                      const char* value) {
    auto mount = get_instance(instance_ptr);
    return mount->cfg->Set(key, value);
}

int curvefs_mount(uintptr_t instance_ptr,
                  const char* fsname,
                  const char* mountpoint) {
    auto mount = get_instance(instance_ptr);
    auto rc = mount->vfs->Mount(fsname, mountpoint, mount->cfg);
    return SysErr(rc);
}

int curvefs_umonut(uintptr_t instance_ptr) {
    auto mount = get_instance(instance_ptr);
    auto rc = mount->vfs->Umount();
    return SysErr(rc);
}

int curvefs_mkdir(uintptr_t instance_ptr, const char* path, uint16_t mode) {
    auto mount = get_instance(instance_ptr);
    auto rc = mount->vfs->MkDir(path, mode);
    return SysErr(rc);
}

int curvefs_mkdirs(uintptr_t instance_ptr, const char* path, uint16_t mode) {
    auto mount = get_instance(instance_ptr);
    auto rc = mount->vfs->MkDirs(path, mode);
    return SysErr(rc);
}

int curvefs_rmdir(uintptr_t instance_ptr, const char* path) {
    auto mount = get_instance(instance_ptr);
    auto rc = mount->vfs->RmDir(path);
    return SysErr(rc);
}

int curvefs_opendir(uintptr_t instance_ptr,
                    const char* path,
                    dir_stream_t* dir_stream) {
    DirStream stream;
    auto mount = get_instance(instance_ptr);
    auto rc = mount->vfs->OpenDir(path, &stream);
    if (rc == CURVEFS_ERROR::OK) {
        *dir_stream = *reinterpret_cast<dir_stream_t*>(&stream);
    }
    return SysErr(rc);
}

ssize_t curvefs_readdir(uintptr_t instance_ptr,
                        dir_stream_t* dir_stream,
                        dirent_t* dirent) {
    DirEntry dirEntry;
    auto mount = get_instance(instance_ptr);
    DirStream* stream = reinterpret_cast<DirStream*>(dir_stream);
    auto rc = mount->vfs->ReadDir(stream, &dirEntry);
    if (rc == CURVEFS_ERROR::OK) {
        strcpy(dirent->name, dirEntry.name.c_str());
        mount->vfs->Attr2Stat(&dirEntry.attr, &dirent->stat);
        return 1;
    } else if (rc == CURVEFS_ERROR::END_OF_FILE) {
        return 0;
    }
    return SysErr(rc);
}

int curvefs_closedir(uintptr_t instance_ptr, dir_stream_t* dir_stream) {
    DirStream* stream = reinterpret_cast<DirStream*>(dir_stream);
    auto mount = get_instance(instance_ptr);
    auto rc = mount->vfs->CloseDir(stream);
    return SysErr(rc);
}

int curvefs_open(uintptr_t instance_ptr,
                 const char* path,
                 uint32_t flags,
                 uint16_t mode) {
    CURVEFS_ERROR rc;
    auto mount = get_instance(instance_ptr);
    if (flags & O_CREAT) {
        rc = mount->vfs->Create(path, mode);
        if (rc != CURVEFS_ERROR::OK) {
            return SysErr(rc);
        }
    }

    uint64_t fd;
    rc = mount->vfs->Open(path, flags, mode, &fd);
    if (rc != CURVEFS_ERROR::OK) {
        return SysErr(rc);
    }
    return static_cast<int>(fd);
}

int curvefs_lseek(uintptr_t instance_ptr,
                  int fd,
                  uint64_t offset,
                  int whence) {
    auto mount = get_instance(instance_ptr);
    auto rc = mount->vfs->LSeek(fd, offset, whence);
    return SysErr(rc);
}

ssize_t curvefs_read(uintptr_t instance_ptr,
                     int fd,
                     char* buffer,
                     size_t count) {
    size_t nread;
    auto mount = get_instance(instance_ptr);
    auto rc = mount->vfs->Read(fd, buffer, count, &nread);
    if (rc != CURVEFS_ERROR::OK) {
        return SysErr(rc);
    }
    return nread;
}

ssize_t curvefs_write(uintptr_t instance_ptr,
                      int fd,
                      char* buffer,
                      size_t count) {
    size_t nwritten;
    auto mount = get_instance(instance_ptr);
    auto rc = mount->vfs->Write(fd, buffer, count, &nwritten);
    if (rc != CURVEFS_ERROR::OK) {
        return SysErr(rc);
    }
    return nwritten;
}

int curvefs_fsync(uintptr_t instance_ptr, int fd) {
    auto mount = get_instance(instance_ptr);
    auto rc = mount->vfs->FSync(fd);
    return SysErr(rc);
}

int curvefs_close(uintptr_t instance_ptr, int fd) {
    auto mount = get_instance(instance_ptr);
    auto rc = mount->vfs->Close(fd);
    return SysErr(rc);
}

int curvefs_unlink(uintptr_t instance_ptr, const char* path) {
    auto mount = get_instance(instance_ptr);
    auto rc = mount->vfs->Unlink(path);
    return SysErr(rc);
}

int curvefs_statfs(uintptr_t instance_ptr,
                   const char* path,
                   struct statvfs* statvfs) {
    auto mount = get_instance(instance_ptr);
    auto rc = mount->vfs->StatFS(path, statvfs);
    return SysErr(rc);
}

int curvefs_lstat(uintptr_t instance_ptr, const char* path, struct stat* stat) {
    auto mount = get_instance(instance_ptr);
    auto rc = mount->vfs->LStat(path, stat);
    return SysErr(rc);
}

int curvefs_fstat(uintptr_t instance_ptr, int fd, struct stat* stat) {
    auto mount = get_instance(instance_ptr);
    auto rc = mount->vfs->FStat(fd, stat);
    return SysErr(rc);
}

int curvefs_setattr(uintptr_t instance_ptr,
                    const char* path,
                    struct stat* stat,
                    int to_set) {
    auto mount = get_instance(instance_ptr);
    auto rc = mount->vfs->SetAttr(path, stat, to_set);
    return SysErr(rc);
}

int curvefs_chmod(uintptr_t instance_ptr, const char* path, uint16_t mode) {
    auto mount = get_instance(instance_ptr);
    auto rc = mount->vfs->Chmod(path, mode);
    return SysErr(rc);
}

int curvefs_rename(uintptr_t instance_ptr,
                   const char* oldpath,
                   const char* newpath) {
    auto mount = get_instance(instance_ptr);
    auto rc = mount->vfs->Rename(oldpath, newpath);
    return SysErr(rc);
}

std::string curvefs_lookup_owner(uintptr_t instance_ptr, uint32_t uid) {
    auto mount = get_instance(instance_ptr);
    return mount->uid2name(uid);
}

std::string curvefs_lookup_group(uintptr_t instance_ptr, uint32_t gid) {
    auto mount = get_instance(instance_ptr);
    return mount->gid2name(gid);
}
