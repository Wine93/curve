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
 * Created Date: 2023-07-30
 * Author: Cao Xianfei
 */

#ifndef CURVEFS_SDK_LIBCURVEFS_GUID_H_
#define CURVEFS_SDK_LIBCURVEFS_GUID_H_

#include <string>
#include <memory>
#include <map>
#include <vector>

#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <pwd.h>
#include <grp.h>
#include <openssl/md5.h>

struct pwent {
    uint32_t id;
    std::string name;
};

class Mapping {
public:
    Mapping(std::string salt): salt(salt) {}

    static Mapping newMapping(std::string salt);
    uint32_t lookupUser(std::string user);
    uint32_t lookupGroup(std::string groups);
    void update(const std::vector<pwent>& uids, const std::vector<pwent>& gids, bool local);

private:
    uint32_t genGuid(std::string name);

private:
    std::string salt;      
    bool local;     
    uint32_t mask;      
    std::map<std::string, uint32_t> usernames; 
    std::map<uint32_t, std::string> userIDs;   
    std::map<std::string, uint32_t> groups;    
    std::map<uint32_t, std::string> groupIDs;  
};

#endif // CURVEFS_SDK_LIBCURVEFS_GUID_H_