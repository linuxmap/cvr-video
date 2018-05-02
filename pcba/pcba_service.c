/**
 * Copyright (C) 2016 Fuzhou Rockchip Electronics Co., Ltd
 * author: Benjo Lei <benjo.lei@rock-chips.com>
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

#ifdef _PCBA_SERVICE_

#include <dirent.h>
#include <net/if.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <netdb.h>
#include <ifaddrs.h>

#include "pcba_rec.h"
#include "pthread.h"
#include "parameter.h"
#include "public_interface.h"
#include "system.h"

#include "ueventmonitor/usb_sd_ctrl.h"
#include "wifi_api.h"

#define CMD_START_TEST_CAMERA_STR "CMD_START_TEST_CAMERA"
#define _STRI(s) #s
#define STRI(s) _STRI(s)

struct type_cmd_func {
    int (*func)(int nfd, char *str);
    char *cmd;
};

typedef struct _pic_thread_tp {
    void * buff;
    int width;
    int height;
    size_t size;
} pic_thread_tp;

#define BUFF_SIZE_S 64
#define BUFF_SIZE_M 128
#define BUFF_SIZE_L 256

#define UDPPORT 18089
#define TCPPORT 8808
#define PICPORT 8807
#define MICPORT 8806

static int broadcast_fd = -1;
static int tcp_server_fd = -1;
static int connect_server_fd = -1;
static int camera_data_accept = - 1;
static int mic_data_accept = - 1;
static int camera_data_fd = - 1;
static int mic_data_fd = - 1;

static pthread_t broadcast_tid;
static pthread_t tcp_server_tid;
static pthread_t pic_server_tid;

static volatile int g_pcba_is_running;
static volatile int g_pcba_is_send_pic;
char SSID[33];
char PASSWD[65];
static pic_thread_tp pic_dat;

static int cmd_resolve(int fd, char *cmdstr);

static int linux_check_network_card(char * network_card)
{
    struct ifaddrs *ifAddrStruct;
    getifaddrs(&ifAddrStruct);
    while (ifAddrStruct != NULL) {
        if (strcmp(ifAddrStruct->ifa_name, network_card) == 0)
            return 1;

        ifAddrStruct = ifAddrStruct->ifa_next;
    }
    freeifaddrs(ifAddrStruct);
    return 0;
}

static int linux_get_mac(char *mac)
{

    int sockfd;
    struct ifreq tmp;
    char mac_addr[30] = {0};

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("create socket fail");
        return -1;
    }

    memset(&tmp, 0, sizeof(struct ifreq));
    strncpy(tmp.ifr_name, "wlan0", sizeof(tmp.ifr_name) - 1);
    if ((ioctl(sockfd, SIOCGIFHWADDR, &tmp)) < 0) {
        printf("mac ioctl error\n");
        return -1;
    }

    snprintf(mac_addr, sizeof(mac_addr), "%02x%02x%02x%02x%02x%02x",
             (unsigned char)tmp.ifr_hwaddr.sa_data[0],
             (unsigned char)tmp.ifr_hwaddr.sa_data[1],
             (unsigned char)tmp.ifr_hwaddr.sa_data[2],
             (unsigned char)tmp.ifr_hwaddr.sa_data[3],
             (unsigned char)tmp.ifr_hwaddr.sa_data[4],
             (unsigned char)tmp.ifr_hwaddr.sa_data[5]
            );
    close(sockfd);
    memcpy(mac, mac_addr, strlen(mac_addr));

    return 0;
}

static void pcba_msg_manager_cb(void *msgData, void *msg0, void *msg1)
{
    struct public_message msg;
    char buff[256] = {0};
    long long tf_free;
    long long tf_total;

    memset(&msg, 0, sizeof(msg));
    if (NULL != msgData) {
        msg.id = ((struct public_message *)msgData)->id;
        msg.type = ((struct public_message *)msgData)->type;
    }

    if (msg.type != TYPE_WIFI && msg.type != TYPE_BROADCAST)
        return;

    switch (msg.id) {
    case MSG_KEY_EVENT_INFO:
        snprintf(buff, sizeof(buff), "CMD_PCBA_KEY:%d", (int)msg1);
        break;

    case MSG_SDCORD_CHANGE:
        printf("MSG_SDCORD_CHANGE\n");
        if (api_get_sd_mount_state() == SD_STATE_IN) {
            printf("sd_state_in\n");
            fs_manage_getsdcardcapacity(&tf_free, &tf_total);
            snprintf(buff, sizeof(buff), "CMD_PCBA_SDCARD:%4.1fG/%4.1fG", (float)tf_free / 1024,
                     (float)tf_total / 1024);
        }
        break;
    default:
        return;
    }

    if ( connect_server_fd < 0 ) {
        printf("pcba_msg_manager_cb connect_server_fd < 0... \n");
        return;
    }

    if (write(connect_server_fd, buff, strlen(buff)) < 0)
        perror("pcba_msg_manager_cb write fail");
}

static void * pic_server_thread(void *arg)
{
    char str[128] = {0};
    int size, len = 0;
    char * buff;

    pthread_detach(pthread_self());

    while (g_pcba_is_running) {
        if (g_pcba_is_send_pic == 0) {
            continue;
        }
        g_pcba_is_send_pic = 0;
        video_cancellation_pcba_callback();

        if (camera_data_fd < 0)
            goto end;

        if (pic_dat.buff == NULL) {
            goto end;
        }

        size = pic_dat.size;
        buff = pic_dat.buff;
        while (size > 0) {
            len = write(camera_data_fd, buff, 1024);
            if (len < 0) {
                perror("write");
                goto end;
            }
            buff += len;
            size -= len;
        }

        if ( connect_server_fd < 0 ) {
            printf("pcba_ion_send_service connect_server_fd < 0... \n");
            goto end;
        }
        snprintf(str, sizeof(str), "CMD_TS_CAMERA_DATA_FINISH:%d*%d", pic_dat.width, pic_dat.height);

        if (write(connect_server_fd, str, strlen(str)) < 0) {
            perror("pcba_ion_send_service write fail!");
            goto end;
        }

    end:
        if (camera_data_fd >= 0) {
            close(camera_data_fd);
            camera_data_fd = -1;
        }
    }
    return NULL;
}

static int pcba_ion_send_service(void * buff, int width, int height, size_t size)
{
    if (g_pcba_is_send_pic) {
        return 0;
    }

    pic_dat.width = width;
    pic_dat.height = height;
    pic_dat.size = size;
    pic_dat.buff = buff;

    g_pcba_is_send_pic = 1;
    return 0;
}

static int tcp_create_picts_service(void)
{
    struct sockaddr_in s_add;
    int opt = 1;

    camera_data_accept = socket(AF_INET, SOCK_STREAM, 0);
    if (camera_data_accept < 0)
        return -1;

    if (setsockopt(camera_data_accept, SOL_SOCKET, SO_REUSEADDR, (char *)&opt,
                   sizeof(opt)) < 0) {
        shutdown(camera_data_accept, 2);
        close(camera_data_accept);
        camera_data_accept = -1;
        return -1;
    }
    printf("socket ok !\r\n");

    bzero(&s_add, sizeof(struct sockaddr_in));
    s_add.sin_family = AF_INET;
    s_add.sin_addr.s_addr = INADDR_ANY;
    s_add.sin_port = htons(PICPORT);

    while (bind(camera_data_accept, (struct sockaddr *)(&s_add),
                sizeof(struct sockaddr)) != 0) {
        perror("bind");
        sleep(1);
    }
    printf("bind ok !\r\n");

    if (listen(camera_data_accept, 1) < 0) {
        perror("listen");
        shutdown(camera_data_accept, 2);
        close(camera_data_accept);
        camera_data_accept = -1;
        return -1;
    }

    if (pthread_create(&pic_server_tid, NULL, pic_server_thread, NULL) != 0) {
        perror("pthread_create");
        if (camera_data_fd > 0) {
            shutdown(camera_data_fd, 2);
            close(camera_data_fd);
            camera_data_fd = -1;
        }
        return -1;
    }

    return -1;
}

static int tcp_create_mic_service(void)
{
    struct sockaddr_in s_add;
    int opt = 1;

    mic_data_accept = socket(AF_INET, SOCK_STREAM, 0);
    if (mic_data_accept < 0) {
        return -1;
    }

    if (setsockopt(mic_data_accept, SOL_SOCKET, SO_REUSEADDR, (char *)&opt,
                   sizeof(opt)) < 0) {
        shutdown(mic_data_accept, 2);
        close(mic_data_accept);
        return -1;
    }
    printf("picts socket ok !\r\n");

    bzero(&s_add, sizeof(struct sockaddr_in));
    s_add.sin_family = AF_INET;
    s_add.sin_addr.s_addr = INADDR_ANY;
    s_add.sin_port = htons(MICPORT);

    while (bind(mic_data_accept, (struct sockaddr *)(&s_add),
                sizeof(struct sockaddr)) != 0) {
        perror("picts bind fail");
        sleep(1);
    }
    printf("picts bind ok !\r\n");

    if (listen(mic_data_accept, 1) < 0) {
        perror("picts listen fail");
        close(mic_data_accept);
        return -1;
    }

    return -1;
}

static void *broadcast_thread(void *arg)
{
    socklen_t addr_len = 0;
    char buf[64];
    char name[64];
    size_t len = sizeof(name);
    struct sockaddr_in server_addr;
    char wifi_info[128];
    int opt = 1;
    struct timeval timeout = {3, 0};
    int ret = -1;

    pthread_detach(pthread_self());
    broadcast_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (broadcast_fd < 0) {
        perror("socket");
        pthread_exit(NULL);
    }
    printf("broadcast_thread socketfd = %d\n", broadcast_fd);

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(UDPPORT);
    addr_len = sizeof(server_addr);

    if (setsockopt(broadcast_fd, SOL_SOCKET, SO_BROADCAST, (char *)&opt,
                   sizeof(opt)) < 0) {
        perror("setsockopt SO_BROADCAST");
        goto stop_broadcast_thread;
    }

    while (bind(broadcast_fd, (struct sockaddr *)&server_addr, addr_len) != 0) {
        perror("broadcast_thread bind");
        sleep(1);
    }

    setsockopt(broadcast_fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(struct timeval));
    gethostname(name, len);

    printf("Wait Discover....\n");
    while (g_pcba_is_running) {
        addr_len = sizeof(server_addr);
        memset(&server_addr, 0, sizeof(server_addr));
        memset(buf, 0, sizeof(buf));
        if (recvfrom(broadcast_fd, buf, 64, 0,
                     (struct sockaddr *)&server_addr, &addr_len) < 0) {
            if (g_pcba_is_running) {
                continue;
            } else
                goto stop_broadcast_thread;
        }

        printf("broadcast_thread : from: %s port: %d > %s\n",
               inet_ntoa(server_addr.sin_addr),
               ntohs(server_addr.sin_port), buf);

    loop_broadcast_discover:
        if (strcmp("CMD_DISCOVER", buf) == 0) {
            memset(wifi_info, 0, sizeof(wifi_info));

            strcat(wifi_info, "CMD_DISCOVER:NAME:");
            strcat(wifi_info, SSID);

            strcat(wifi_info, "MAC:");
            linux_get_mac(&wifi_info[strlen(wifi_info)]);

            strcat(wifi_info, "SID:0000-0000-0000-0000");
            strcat(wifi_info, "END");

            addr_len = sizeof(server_addr);
            printf("sendto: %s port: %d > %s\n",
                   inet_ntoa(server_addr.sin_addr),
                   ntohs(server_addr.sin_port), wifi_info);
        loop_broadcast_send:
            if (sendto(broadcast_fd, wifi_info, strlen(wifi_info),
                       0, (struct sockaddr *)&server_addr, addr_len) < 0)
                goto stop_broadcast_thread;

            memset(buf, 0, sizeof(buf));

            ret = recvfrom(broadcast_fd, buf, 64, 0,
                           (struct sockaddr *)&server_addr, &addr_len);
            if (ret < 0) {
                if (g_pcba_is_running) {
                    printf("udp recv ack timeout... %d\n", ret);
                    goto loop_broadcast_send;
                } else
                    goto stop_broadcast_thread;
            } else {
                printf("recv ack : %s\n", buf);
                if (strcmp("CMD_ACK_DISCOVER", buf) == 0)
                    continue;
                else
                    goto loop_broadcast_discover;
            }
        }
    }

stop_broadcast_thread:
    printf("stop broadcast_thread\n");
    if (broadcast_fd >= 0) {
        shutdown(broadcast_fd, 2);
        close(broadcast_fd);
        broadcast_fd = -1;
    }
    pthread_exit(NULL);
    return NULL;
}

static void *tcp_receive(void *arg)
{
    int nfp = (int)arg;
    int recbytes;
    char buffer[512] = {0};
    char data[512] = {0};

    pthread_detach(pthread_self());
    printf("tcp_receive : tcp_receive nfp = 0x%x\n", nfp);
    while (g_pcba_is_running) {
        memset(buffer, 0, sizeof(buffer));
        memset(data, 0, sizeof(data));

        printf("tcp_receive : Wait TCP CMD.....\n");

        recbytes = read(nfp, buffer, sizeof(buffer));
        if (0 == recbytes) {
            printf("tcp_receive : disconnect!\r\n");
            goto stop_tcp_receive;
        }

        buffer[recbytes] = '\0';

        cmd_resolve(nfp, buffer);
    }

stop_tcp_receive:
    shutdown(nfp, 2);
    close(nfp);
    pthread_exit(NULL);
    return NULL;
}

static void *tcp_server_thread(void *arg)
{
    int nfp = 0;
    struct sockaddr_in s_add, c_add;
    socklen_t sin_size;
    pthread_t tcpreceive_tid;
    int opt = 1;

    pthread_detach(pthread_self());

    tcp_create_picts_service();
    tcp_create_mic_service();
    tcp_server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (tcp_server_fd < 0) {
        printf("tcp_server_thread : socket fail ! \r\n");
        pthread_exit(NULL);
        return NULL;
    }
    if (setsockopt(tcp_server_fd, SOL_SOCKET, SO_REUSEADDR, (char *)&opt,
                   sizeof(opt)) < 0) {
        perror("SO_REUSEADDR");
        shutdown(tcp_server_fd, 2);
        close(tcp_server_fd);
        pthread_exit(NULL);
        return NULL;
    }
    printf("socket ok !\r\n");

    bzero(&s_add, sizeof(struct sockaddr_in));
    s_add.sin_family = AF_INET;
    s_add.sin_addr.s_addr = INADDR_ANY;
    s_add.sin_port = htons(TCPPORT);

    while (bind(tcp_server_fd, (struct sockaddr *)(&s_add),
                sizeof(struct sockaddr)) != 0) {
        perror("bind fail");
        sleep(1);
    }
    printf("tcp_server_thread : bind ok !\r\n");

    if (listen(tcp_server_fd, 1) < 0) {
        perror("listen fail");
        goto stop_tcp_server_thread;
    }
    printf("listen ok\r\n");
    while (g_pcba_is_running) {
        sin_size = sizeof(struct sockaddr_in);

        nfp = accept(tcp_server_fd, (struct sockaddr *)(&c_add),
                     &sin_size);
        if (nfp < 0) {
            perror("accept fail!");
            goto stop_tcp_server_thread;
        }

        connect_server_fd = nfp;
        printf("accept ok!\r\nServer get connect from %x : %x\r\n",
               ntohl(c_add.sin_addr.s_addr), ntohs(c_add.sin_port));

        if (pthread_create(&tcpreceive_tid, NULL, tcp_receive,
                           (void *)nfp) != 0) {
            perror("Create thread error!");
            goto stop_tcp_server_thread;
        }

        printf("tcpreceive TID in pthread_create function: %x.\n",
               (int)tcpreceive_tid);
    }

stop_tcp_server_thread:
    if (tcp_server_fd >= 0) {
        shutdown(tcp_server_fd, 2);
        close(tcp_server_fd);
        tcp_server_fd = -1;
    }

    if (nfp > 0) {
        shutdown(nfp, 2);
        close(nfp);
        connect_server_fd = -1;
    }
    pthread_exit(NULL);
    return NULL;
}

static void socket_thread_create(void)
{
    if (pthread_create(&broadcast_tid, NULL, broadcast_thread, NULL) != 0)
        printf("Create thread error!\n");

    if (pthread_create(&tcp_server_tid, NULL, tcp_server_thread, NULL) != 0)
        printf("Createthread error!\n");
}

static void pcba_wifi_init(void)
{
    WlanServiceSetPower("on");

    parameter_sav_wifi_mode(1);

#if defined(_PCBA_SERVICE_WIFI_SSID_) && defined(_PCBA_SERVICE_WIFI_PASSWD_)
    parameter_sav_staewifiinfo(STRI(_PCBA_SERVICE_WIFI_SSID_), STRI(_PCBA_SERVICE_WIFI_PASSWD_));
#else
    parameter_sav_staewifiinfo("RV_PCBA", "12345678");
#endif

    wifi_deamon_thread_start(WIFI_MODE_STA);
    parameter_getwifistainfo(SSID, PASSWD);
    WlanServiceSetMode("station", SSID, PASSWD);
}

static void pcba_net_init(void)
{
    runapp("ifconfig eth0 up");
    runapp("udhcpc &");
}

static void pcba_rndis_init(void)
{
    runapp("echo 1 >/tmp/RV_USB_STATE_MASK_DEBUG");
    runapp("echo RockChip > /sys/class/android_usb/android0/iManufacturer");
    runapp("echo RV1108 > /sys/class/android_usb/android0/iProduct");
    runapp("echo 0 > /sys/class/android_usb/android0/enable");
    runapp("echo 2207 > /sys/class/android_usb/android0/idVendor");
    runapp("echo 0007 > /sys/class/android_usb/android0/idProduct");
    runapp("echo rndis > /sys/class/android_usb/android0/functions");
    runapp("echo 1 > /sys/class/android_usb/android0/enable");

    runapp("ifconfig lo 127.0.0.1 netmask 255.255.255.0");
    runapp("ifconfig rndis0 192.168.200.1 netmask 255.255.255.0 up");
    runapp("route add default gw 192.168.200.1 rndis0");

    runapp("echo \"interface=rndis0\" > /tmp/rndis_gdb_dnsmasq.conf");
    runapp("echo \"user=root\" >> /tmp/rndis_gdb_dnsmasq.conf");
    runapp("echo \"listen-address=192.168.200.1\" >> /tmp/rndis_gdb_dnsmasq.conf");
    runapp("echo \"dhcp-range=192.168.200.50,192.168.200.200,24h\" >> /tmp/rndis_gdb_dnsmasq.conf");
    runapp("echo \"server=/google/8.8.8.8\" >> /tmp/rndis_gdb_dnsmasq.conf");
    runapp("/usr/local/sbin/dnsmasq -O 6 -C /tmp/rndis_gdb_dnsmasq.conf");
}

void pcba_service_start(void)
{
    if (g_pcba_is_running) {
        printf("pcba_service_start is now runnning, don't start again!\n");
        return;
    }

    g_pcba_is_running = 1;

    printf("This is pcba_service_start\n");

#ifdef _PCBA_SERVICE_NETWORK_
    printf("Init network....\n");
    pcba_net_init();
#endif

#ifdef _PCBA_SERVICE_WIFI_
    printf("Init wifi....\n");
    pcba_wifi_init();
#endif

#ifdef _PCBA_SERVICE_RNDIS_
    printf("Init rndis....\n");
    pcba_rndis_init();
#endif

    parameter_getwifistainfo(SSID, PASSWD);
    reg_entry(pcba_msg_manager_cb);
    if (g_pcba_is_running)
        socket_thread_create();
}

static void socket_thread_close(void)
{
    int temp;
    if (broadcast_fd >= 0) {
        temp = broadcast_fd;
        broadcast_fd = -1;
        shutdown(temp, 2);
        close(temp);

    }
    if (tcp_server_fd >= 0) {
        temp = tcp_server_fd;
        tcp_server_fd = -1;
        shutdown(temp, 2);
        close(temp);
    }

    if (camera_data_accept >= 0) {
        temp = camera_data_accept;
        camera_data_accept = -1;
        shutdown(temp, 2);
        close(temp);
    }

    if (camera_data_fd > 0) {
        temp = camera_data_fd;
        camera_data_fd = -1;
        shutdown(temp, 2);
        close(temp);
    }

    if (mic_data_accept >= 0) {
        temp = mic_data_accept;
        mic_data_accept = -1;
        shutdown(temp, 2);
        close(temp);
    }
}

void pcba_service_stop(void)
{
    if (!g_pcba_is_running) {
        printf("pcba is not runnning, cannot stop it!\n");
        return;
    }

    g_pcba_is_running = 0;

    if (!g_pcba_is_running)
        socket_thread_close();

    WlanServiceSetPower("off");
}

static int tcp_func_test_audio(int nfp, char *str)
{
    audio_sync_play("/usr/local/share/sounds/pcba.wav");

    if (write(nfp, "CMD_ACK_TEST_AUDIO", sizeof("CMD_ACK_TEST_AUDIO")) < 0) {
        perror("write");
    }
    return 0;
}

static int tcp_func_test_mic_start(int nfp, char *str)
{
    struct sockaddr_in c_add;
    socklen_t sin_size;
    char * type;

    sin_size = sizeof(struct sockaddr_in);
    mic_data_fd = accept(mic_data_accept, (struct sockaddr *)(&c_add),
                         &sin_size);
    if (mic_data_fd < 0) {
        perror("picts listen fail");
        close(mic_data_accept);
        return -1;
    }

    if (write(nfp, "CMD_ACK_MIC_TEST", sizeof("CMD_ACK_MIC_TEST")) < 0) {
        perror("write fail");
    }

    pcba_register(mic_data_fd);
    return 0;
}

static int tcp_func_test_mic_stop(int nfp, char *str)
{
    pcba_unregister();
    if (write(nfp, "CMD_ACK_MIC_STOP_TEST", sizeof("CMD_ACK_MIC_STOP_TEST")) < 0) {
        perror("write");
    }
    return 0;
}

static int tcp_func_test_net(int nfp, char *str)
{
    char buff[128] = "CMD_ACK_NET_TEST:";
    if (linux_check_network_card("eth0")) {
        strcat(buff, "eth0&");
    }

    if (linux_check_network_card("wlan0")) {
        strcat(buff, "wlan0&");
    }

    if (write(nfp, buff, strlen(buff)) < 0) {
        perror("write");
    }
    return 0;
}

static int tcp_func_test_camera_start(int nfp, char *str)
{
    struct sockaddr_in c_add;
    socklen_t sin_size;
    char * type;
    int camera_test_type = -1;

    printf("benjo.lei(2.0) ......\n");

    type = &str[sizeof(CMD_START_TEST_CAMERA_STR)];
    if ( strcmp(type, "ISP") == 0 )
        camera_test_type = 1;
    else if ( strcmp(type, "USB") == 0 )
        camera_test_type = 2;
    else if ( strcmp(type, "CIF1") == 0 )
        camera_test_type = 3;
    else if ( strcmp(type, "CIF2") == 0 )
        camera_test_type = 4;
    else if ( strcmp(type, "CIF3") == 0 )
        camera_test_type = 5;
    else if ( strcmp(type, "CIF4") == 0 )
        camera_test_type = 6;
    else
        camera_test_type = 0;

    sin_size = sizeof(struct sockaddr_in);

    if (camera_data_fd > 0) {
        int tmp = camera_data_fd;
        camera_data_fd = -1;
        shutdown(tmp, 2);
        close(tmp);
    }

    camera_data_fd = accept(camera_data_accept, (struct sockaddr *)(&c_add),
                            &sin_size);
    if (camera_data_fd <= 0) {
        perror("listen");
        return -1;
    }

    if (write(nfp, "CMD_ACK_CAMERA_TEST", sizeof("CMD_ACK_CAMERA_TEST")) < 0) {
        perror("write");
        int tmp = camera_data_fd;
        camera_data_fd = -1;
        shutdown(tmp, 2);
        close(tmp);
        return -1;
    }

    video_register_pcba_callback(camera_test_type, pcba_ion_send_service);
    return 0;
}

// The command must start with "CMD_".
#define CMD_PREFIX "CMD_"
static const struct type_cmd_func cmd_tab[] = {
    {tcp_func_test_camera_start,       "CMD_START_TEST_CAMERA",},
//    {tcp_func_test_camera_stop,        "CMD_STOP_TEST_CAMERA",},
    {tcp_func_test_net,                "CMD_TEST_NET",},
    {tcp_func_test_audio,              "CMD_TEST_AUDIO",},
    {tcp_func_test_mic_start,          "CMD_TEST_MIC_START",},
    {tcp_func_test_mic_stop,           "CMD_TEST_MIC_STOP",},
};

static int cmd_resolve(int fd, char *cmdstr)
{
    size_t i = 0;

    printf("cmd_resolve = %s\n", cmdstr);

    for (i = 0; i < sizeof(cmd_tab) / sizeof(cmd_tab[1]); i++) {
        char *result = strstr(cmdstr, CMD_PREFIX);
        if (result != NULL) {
            size_t cmdlen = strlen(cmd_tab[i].cmd);
            if (strncmp(result, cmd_tab[i].cmd, cmdlen) == 0) {
                if (cmd_tab[i].func != NULL)
                    cmd_tab[i].func(fd, result);
                cmdstr = result + cmdlen;
                if (*cmdstr == '\0' || strstr(cmdstr, CMD_PREFIX) == NULL)
                    return 0;
                printf("handle continuous cmd: %s\n", cmdstr);
                i = 0;
            }
        }
    }
    return -1;
}
#endif

