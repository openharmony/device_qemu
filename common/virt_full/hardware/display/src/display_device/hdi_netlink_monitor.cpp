/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "hdi_netlink_monitor.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include "hdi_session.h"

namespace OHOS {
namespace HDI {
namespace DISPLAY {
HdiNetLinkMonitor::HdiNetLinkMonitor()
{
    DISPLAY_LOGD();
}

int HdiNetLinkMonitor::Init()
{
    DISPLAY_LOGD();
    mThread = std::make_unique<std::thread>(std::bind(&HdiNetLinkMonitor::MonitorThread, this));
    mThread->detach();
    mRunning = true;
    return DISPLAY_SUCCESS;
}

HdiNetLinkMonitor::~HdiNetLinkMonitor()
{
    DISPLAY_LOGD();
    if (mScoketFd >= 0) {
        close(mScoketFd);
    }
}

static int ThreadInit()
{
    int ret;
    int fd = -1;
    const int32_t bufferSize = 1024;
    struct sockaddr_nl snl;

    bzero(&snl, sizeof(struct sockaddr_nl));
    snl.nl_family = AF_NETLINK;
    snl.nl_pid = getpid();
    snl.nl_groups = 1;
    fd = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_KOBJECT_UEVENT);
    DISPLAY_CHK_RETURN(fd < 0, DISPLAY_FAILURE, DISPLAY_LOGE("socket fail"));
    ret = setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &bufferSize, sizeof(bufferSize));
    if (ret == -1) {
        DISPLAY_LOGE("setsockopt fail");
        close(fd);
        return -1;
    }
    ret = bind(fd, reinterpret_cast<struct sockaddr *>(&snl), sizeof(struct sockaddr_nl));
    if (ret < 0) {
        DISPLAY_LOGE("bind fail");
        close(fd);
        return -1;
    }
    return fd;
}

static void ParseUeventMessage(const char *buf, uint32_t len)
{
    uint32_t num;

    for (num = 0; num < len;) {
        const char *event = buf + num;
        if (strcmp(event, "STATE=HDMI=0") == 0) {
            HdiSession::GetInstance().HandleHotplug(false);
            break;
        } else if (strcmp(event, "STATE=HDMI=1") == 0) {
            HdiSession::GetInstance().HandleHotplug(true);
            break;
        }
        num += strlen(event) + 1;
    }
}

void HdiNetLinkMonitor::MonitorThread()
{
    constexpr int BUFFER_SIZE = 2048; /* buffer for the variables */
    int len;
    int fd = -1;

    fd = ThreadInit();
    DISPLAY_CHK_RETURN_NOT_VALUE(fd < 0, DISPLAY_LOGE("socket fail"));
    mScoketFd = fd;
    while (mRunning) {
        char buf[BUFFER_SIZE] = { 0 };
        len = read(fd, &buf, sizeof(buf));
        if (len < 0) {
            DISPLAY_LOGE("read uevent message fail");
            break;
        }
        ParseUeventMessage(buf, len);
    }
}
} // DISPLAY
}  // HDI
}  // OHOS
