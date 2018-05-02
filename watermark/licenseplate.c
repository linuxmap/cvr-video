/**
 * Copyright (C) 2017 Fuzhou Rockchip Electronics Co., Ltd
 * author: Chad.ma <chad.ma@rock-chips.com>
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

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "parameter.h"
#include "ui_resolution.h"
#include "watermark.h"
#include "licenseplate.h"
#include "video.h"

/* Chinese index */
extern uint32_t lic_chinese_idx;
/* Charactor or number index */
extern uint32_t lic_num_idx;
extern uint32_t licplate_pos[MAX_LICN_NUM];

char licenseplate_str[20];
const char g_license_chinese[PROVINCE_ABBR_MAX][ONE_CHN_SIZE] = {
    " ", "¾©", "½ò", "»¦", "Óå", "¼½", "½ú", "ÃÉ", "ÁÉ", "¼ª", "ºÚ",
    "ËÕ", "Õã", "Íî", "Ãö", "¸Ó", "Â³", "Ô¥", "¶õ", "Ïæ", "ÔÁ",
    "¹ð", "Çí", "´¨", "¹ó", "ÔÆ", "²Ø", "Çà", "ÉÂ", "¸Ê", "Äþ",
    "ÐÂ", "¸Û", "°Ä", "Ì¨", "*", "-"
};

const char g_license_char[LICENSE_CHAR_MAX][ONE_CHAR_SIZE] = {
    " ", "0", "1", "2", "3", "4", "5", "6", "7", "8", "9",
    "A", "B", "C", "D", "E", "F", "G", "H", "I", "J",
    "K", "L", "M", "N", "O", "P", "Q", "R", "S", "T",
    "U", "V", "W", "X", "Y", "Z", "*", "-",
};

bool is_licenseplate_valid(char *licenStr)
{
    int i;
    char c0 = ' ', c1 = '-', c2 = '*';
    char *temp;

    if (licenStr == NULL)
        return false;

    for (i = 0; i < MAX_LICN_NUM; i++) {
        temp = (char *)(licenStr + i * ONE_CHN_SIZE);

        if (temp[0] == c0 || temp[0] == c1 || temp[0] == c2)
            continue;
        else
            break;
    }

    if (i == MAX_LICN_NUM)
        return false;

    return true;
}

void save_licenseplate(char *licnplate)
{
    uint32_t i, pos;
    uint32_t license_len;
    char *temp = NULL;
    uint32_t show_license[MAX_TIME_LEN] = { 0 };

    memset(licenseplate_str, 0, sizeof(licenseplate_str));

    /* Save the content before change fouce CtrlID */
    if (lic_chinese_idx) {
        /* Save the fisrt Chinese Charactor */
        pos = licplate_pos[0];
        temp = (char *)licnplate;
        strcpy(temp, (char *)g_license_chinese[pos]);
        strcat(licenseplate_str, temp);
    }

    for (i = 1; i < MAX_LICN_NUM; i++) {
        if (licplate_pos[i] != 0) {
            pos = licplate_pos[i];
            temp = (char *)(licnplate + i * ONE_CHN_SIZE);
            strcpy(temp, g_license_char[pos]);
            strcat(licenseplate_str, temp);
        }
    }

    parameter_save_licence_plate((char *)licnplate, MAX_LICN_NUM);
    parameter_save_licence_plate_flag(1);

    if (!is_licenseplate_valid(licnplate))
        return;

    watermark_get_show_license(show_license, licenseplate_str, &license_len);
    printf("## license is : %s ,len = %d #\n", licenseplate_str, license_len);
    video_record_update_license(show_license, license_len);
}

void get_licenseplate_and_pos(char *licnplate, int plate_size, uint32_t *licn_pos)
{
    int i, j;
    char *temp;
    char **lic_plate = NULL;

    temp = (char *)licnplate;

    if (parameter_get_licence_plate_flag() == 1) {
        lic_plate = (char **)parameter_get_licence_plate();
        memcpy(temp, (char *)lic_plate, plate_size);
    } else {
        for (i = 0; i < MAX_LICN_NUM; i++) {
            temp = (char *)(licnplate + i * ONE_CHN_SIZE);
            strcpy(temp, "-");
        }
    }

    /* Init the license plate position */
    memset(licn_pos, 0, sizeof(licn_pos[0]) * MAX_LICN_NUM);

    if (parameter_get_licence_plate_flag() != 1)
        return;

    temp = (char *)licnplate;

    for (j = 0; j < PROVINCE_ABBR_MAX; j++) {
        if (!strncmp(temp, g_license_chinese[j], ONE_CHAR_SIZE)) {
            licn_pos[0] = j;
            break;
        }
    }

    for (i = 1; i < MAX_LICN_NUM; i++) {
        temp = (char *)(licnplate + i * ONE_CHN_SIZE);

        for (j = 0; j < LICENSE_CHAR_MAX; j++) {
            if (!strncmp(temp, g_license_char[j], ONE_CHAR_SIZE)) {
                licn_pos[i] = j;
                break;
            }
        }
    }
 }
