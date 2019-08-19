/*
 * Project: curve
 * File Created: 20190830
 * Author: lixiaocui
 * Copyright (c)￼ 2018 netease
 */

#include <vector>
#include <string>
#include "src/mds/nameserver2/allocstatistic/alloc_statistic_helper.h"
#include "src/mds/nameserver2/helper/namespace_helper.h"
#include "proto/nameserver2.pb.h"
#include "src/common/timeutility.h"

namespace curve {
namespace mds {
const int GETBUNDLE = 1000;

int AllocStatisticHelper::GetExistSegmentAllocValues(
    std::map<PoolIdType, uint64_t> *out,
    const std::shared_ptr<EtcdClientImp> &client) {
    // 从etcd中获取logicalPool对应的segmentSize统计值
    std::vector<std::string> allocVec;
    int res = client->List(
        SEGMENTALLOCSIZEKEY, SEGMENTALLOCSIZEKEYEND, &allocVec);
    if (res != EtcdErrCode::OK) {
        LOG(ERROR) << "list [" << SEGMENTALLOCSIZEKEY << ","
                   << SEGMENTALLOCSIZEKEYEND << ") fail, errorCode: "
                   << res;
        return -1;
    }

    // 解析
    for (auto &item : allocVec) {
        PoolIdType lid;
        uint64_t alloc;
        bool res = NameSpaceStorageCodec::DecodeSegmentAllocValue(
            item, &lid, &alloc);
        if (false == res) {
            LOG(ERROR) << "decode segment alloc value: " << item << " fail";
            continue;
        }
        (*out)[lid] = alloc;
    }
    return 0;
}

int AllocStatisticHelper::CalculateSegmentAlloc(
    int64_t revision, const std::shared_ptr<EtcdClientImp> &client,
    std::map<PoolIdType, uint64_t> *out) {
    LOG(INFO) << "start calculate segment alloc, revision: " << revision
              << ", buldle size: " << GETBUNDLE;
    uint64_t startTime = ::curve::common::TimeUtility::GetTimeofDayMs();

    std::string startKey = SEGMENTINFOKEYPREFIX;
    std::vector<std::string> values;
    std::string lastKey;
    do {
        // 清理数据
        values.clear();
        lastKey.clear();

        // 从etcd中批量获取segment
        int res = client->ListWithLimitAndRevision(
           startKey, SEGMENTINFOKEYEND, GETBUNDLE, revision, &values, &lastKey);
        if (res != EtcdErrCode::OK) {
            LOG(ERROR) << "list [" << startKey << "," << SEGMENTINFOKEYEND
                       << ") at revision: " << revision
                       << " with bundle: " << GETBUNDLE
                       << " fail, errCode: " << res;
            return -1;
        }

        // 对获取的值进行解析
        int startPos = 1;
        if (startKey == SEGMENTINFOKEYPREFIX) {
            startPos = 0;
        }
        for ( ; startPos < values.size(); startPos++) {
            PageFileSegment segment;
            bool res = NameSpaceStorageCodec::DecodeSegment(
                values[startPos], &segment);
            if (false == res) {
                LOG(ERROR) << "decode segment item{"
                          << values[startPos] << "} fail";
                return -1;
            } else {
                (*out)[segment.logicalpoolid()] += segment.segmentsize();
            }
        }

        startKey = lastKey;
    } while (values.size() >= GETBUNDLE);

    LOG(INFO) << "calculate segment alloc ok, time spend: "
              << (::curve::common::TimeUtility::GetTimeofDayMs() - startTime)
              << " ms";
    return 0;
}
}  // namespace mds
}  // namespace curve
