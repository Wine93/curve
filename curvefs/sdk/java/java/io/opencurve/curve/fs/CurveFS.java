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

package io.opencurve.curve.fs;

public class CurveFS {
    private long cInstancePtr;

    private static native long nativeCurveFScreate();
    private static native int nativeCurveFSSetUserGroupName(long cInstancePtr, String name, String user, String group, String superuser, String supergroup);
    private static native int nativeCurveFSUpdateUidAndGrouping(long cInstancePtr, String uidStr, String grouping);
    private static native int nativeSetOwner(long cInstancePtr, String path, String user, String group);
    private static native int nativeCurveFSMount(long cInstancePtr);
    private static native int nativeCurveFSMkdir(long cInstancePtr, String path, int mode);
    private static native int nativeCurveFSRmdir(long cInstancePtr, String path);

    static {
        CurveFSNativeLoader.getInstance().loadLibrary();
    }

    public CurveFS() {
        cInstancePtr = nativeCurveFScreate();
    }

    // init->nativeCurveFSSetUserGroupName->(libcurvefs)curvefs_set_uid_grouping->(vfs)setGUid
    public void init(String name, String user, String group, String superuser, String supergroup) {
        nativeCurveFSSetUserGroupName(cInstancePtr, name, user, group, superuser, supergroup);
    }

    // updateUidAndGrouping->nativeCurveFSUpdateUidAndGrouping->(libcurvefs curvefs_update_uid_and_grouping)->(vfs)setGuid
    public void updateUidAndGrouping(String uidStr, String grouping) {
        nativeCurveFSUpdateUidAndGrouping(cInstancePtr, uidStr, grouping);
    }

    // setowner->nativeSetOwner->(libcurvefs curvefs_setowner)->(vfs)Chown
    public void setowner(String path, String user, String group) {
        nativeSetOwner(cInstancePtr, path, user, group);
    }

    public void mount() {
        nativeCurveFSMount(cInstancePtr);
    }

    public void mkdir(String path, int mode) {
        nativeCurveFSMkdir(cInstancePtr, path, mode);
    }

    public void rmdir(String path) {
        nativeCurveFSRmdir(cInstancePtr, path);
    }
};
