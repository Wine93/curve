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

#include "curvefs/sdk/libcurvefs/guid.h"

Mapping newMapping(std::string salt) {
    Mapping m(salt);
    m.update(genAllUids(), genAllGids(), true);
    return m;
}

uint32_t Mapping::lookupUser(std::string user) {
    uint32_t id;
    auto it = usernames.find(user);
    if (it != usernames.end()) {
        return it->second;
    } 
    if (!local) {
        uint32_t id = genGuid(user);
		usernames[user] = id;
		userIDs[id] = user;
		return id;
    }
    if (user == "root") {
        id = genGuid(user); // root in hdfs sdk is a normal user
    } else {
        struct passwd* pw = getpwnam(user.c_str());
        if (pw != nullptr) {
            id = pw->pw_uid;
        } else {
            id = genGuid(user);
        }
    }
    usernames[user] = id;
    userIDs[id] = user;
    return id;
}

uint32_t Mapping::lookupGroup(std::string group){
    uint32_t id;
    auto it = groups.find(group);
    if (it != groups.end()) {
        return it->second;
    } 
    if (!local) {
        uint32_t id = genGuid(group);
		groups[group] = id;
		groupIDs[id] = group;
		return id;
    }
    if (group == "root") {
        id = genGuid(group);
    } else {
        struct group* gr = getgrnam(group.c_str());
        if (gr != nullptr) {
            id = gr->gr_gid;
        } else {
            id = genGuid(group);
        }
    }
    groups[group] = id;
	groupIDs[id] = group;
    return id;
}

void Mapping::update(const std::vector<pwent>& uids, const std::vector<pwent>& gids, bool local){
    this->local = local;
    for (auto &u : uids) {
        uint32_t oldId = usernames[u.name];
		std::string oldName = userIDs[u.id];
        userIDs.erase(oldId);
        usernames.erase(oldName);
		usernames[u.name] = u.id;
		userIDs[u.id] = u.name;
	}
	for (auto &g : gids) {
		uint32_t oldId = groups[g.name];
		std::string oldName = groupIDs[g.id];
        groupIDs.erase(oldId);
        groups.erase(oldName);
		groups[g.name] = g.id;
		groupIDs[g.id] = g.name;
	}
}

uint32_t Mapping::genGuid(std::string name) {
    std::string str = salt + name + salt;
    uint8_t digest[MD5_DIGEST_LENGTH] = { 0 };
    MD5(reinterpret_cast<const uint8_t*>(str.c_str()), str.length(), digest);

    uint64_t a = *reinterpret_cast<uint64_t*>(digest);
    uint64_t b = *reinterpret_cast<uint64_t*>(digest + 8);
    uint32_t id = static_cast<uint32_t>(a ^ b);

    return id;
}

// Gen all uids according local system
std::vector<pwent> genAllUids() {
    struct passwd *pw;
    std::vector<pwent> uids;
    for(;;) {
        pw = getpwent();
        if (pw == NULL) {
            break;
        }
        std::string name(pw->pw_name);
        if (name != "root") {
            uids.push_back(pwent{pw->pw_uid, name});
        }
    }
    return uids;
}

// Gen all gids according local system
std::vector<pwent> genAllGids() {
    struct group *gr;
    std::vector<pwent> gids;
    for(;;) {
        gr = getgrent();
        if (gr == NULL) {
            break;
        }
        std::string name(gr->gr_name);
        if (name != "root") {
            gids.push_back(pwent{gr->gr_gid, name});
        }
    }
    return gids;
}
