/**
 * Copyright (C) 2016 Fuzhou Rockchip Electronics Co., Ltd
 * author: ZhiChao Yu zhichao.yu@rock-chips.com
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "wlan_service_client.h"

#include <stdio.h>
#include <string.h>

#include <string>

#include "librkipc/socket_client.h"

const char* kWlanServiceSocket = "/tmp/socket/wlan_service.sock";

int WlanServiceSetPower(const char* power) {
  rockchip::ipc::SocketClient client;

  client.Connect(kWlanServiceSocket);

  std::string command = std::string("set_power/") + power;
  client.Send(command.c_str());

  client.Disconnect();
  return 0;
}

int WlanServiceSetMode(const char* mode,
                       const char* ssid, const char* password) {
  rockchip::ipc::SocketClient client;

  client.Connect(kWlanServiceSocket);

  if (strcmp(mode, "station") == 0 || strcmp(mode, "ap") == 0) {
    std::string command = std::string("set_mode/") + mode +
                          "/" + ssid + "/" + password + "/";
    client.Send(command.c_str());
  } else {
    printf("Unsupported wlan mode: %s.\n", mode);
  }

  client.Disconnect();
  return 0;
}
