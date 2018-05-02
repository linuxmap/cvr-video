/**
 * Copyright (C) 2017 Fuzhou Rockchip Electronics Co., Ltd
 * Author: Vicent Chi <vicent.chi@rock-chips.com>
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

#include "ril_ATSMS.h"
#include "ril_GB2Utable.h"

static const char HEXCH[] = "0123456789ABCDEF";
static int gMaxSmsSize = 20;
static char gSmsCenter[MAX_PHONE_NUMBER] = "+8613010380500";
static raw_sms_t    raw_pdu;
static raw_sms_t    read_pdu;

static int sms_code_number(char *pSrc, unsigned char *pDst)
{
    int     pos = 0;
    PRINTF_FUN_LINE

    if (*pSrc == '+') {
        pDst[pos++] = 0x91;
        pSrc++;
    } else {
        pDst[pos++] = 0x91; //0x81
        pDst[pos++] = 0x68;
    }

    while (*pSrc) {
        if (*(pSrc + 1)) {
            pDst[pos++] = CHAR2HEX(*pSrc) | (CHAR2HEX(*(pSrc + 1)) << 4);
            pSrc += 2;
        } else {
            pDst[pos++] = CHAR2HEX(*pSrc) | 0xF0;
            pSrc++;
            break;
        }
        if (pos >= MAX_PHONE_NUMBER - 1)
            break;
    }
    pDst[pos] = 0;
    return pos;
}

int IsAsciiString(const char *str)
{
    PRINTF_FUN_LINE
    while (1) {
        if (*str == 0)
            return 1;
        if ((*str < 0x20) || (*str > 0x7E))
            return 0;
        str++;
    }
}

int GSM_Encode7bit(const char* pSrc, unsigned char* pDst, int nSrcLength)
{
    int     nChar;
    unsigned char nLeft, *pEnd;
    PRINTF_FUN_LINE

    nLeft = 0;
    nChar = 0;
    pEnd  = pDst;
    while (nSrcLength--) {
        nChar &= 7;
        if (nChar == 0) {
            nLeft = *pSrc;
        } else {
            *pEnd++ = (*pSrc << (8 - nChar)) | nLeft;
            nLeft = *pSrc >> nChar;
        }
        pSrc++;
        nChar++;
    }
    if (nChar != 8)
        *pEnd++ = nLeft;

    return (int)(pEnd - pDst);
}

WORD SwapWORD(WORD nVal)
{
    BYTE *pSrc, *pDst;
    WORD  nRes;
    PRINTF_FUN_LINE

    pDst = (BYTE*)&nRes;
    pSrc = (BYTE*)&nVal;
    *(pDst + 1) = *pSrc;
    *pDst = *(pSrc + 1);
    return nRes;
}

WORD GB2UniCodeOne(BYTE page, BYTE npos)
{
    int i;
    WORD nRet;
    PRINTF_FUN_LINE

    if ((page < 0xA0) || (page > 0xF7) || (npos < 0xA0)) {
        return 0xA1A1;
    }


    for (i = 0; i < TABLE_NUM; i++) {
        if (((page << 8) + npos) == GB2UC[i][0]) {
            nRet = GB2UC[i][1];
        }
    }

    return SwapWORD(nRet);
}

WORD GB2UniCodeOneEn(BYTE page)
{
    WORD nRet = 0x0;

    PRINTF_FUN_LINE

    if ((page >= 0x21) && (page <= 0x7E)) {

        nRet = GB2UC[(page + TABLE_EN_ARRAY_POS)][1];
    } else {
        return 0xA1A1;
    }

    return SwapWORD(nRet);
}

int GB2UniCode(BYTE *srcStr, WORD *pDst, int num)
{
    int     i, len = 0;
    WORD    uCode;
    BYTE *pDstB;
    PRINTF_FUN_LINE

    for (i = 0; len < num; ++i) {
        if (srcStr[i] == 0)
            break;
        if (srcStr[i] & 0x80 && (srcStr[i + 1] & 0x80)) {
            uCode = GB2UniCodeOne(srcStr[i], srcStr[i + 1]);
            ++i;
        } else {
            uCode = GB2UniCodeOneEn(srcStr[i]);
        }
        *pDst++ = uCode;
        len += 2;
        printf("GB2UniCode uCode:%x\n", SwapWORD(uCode));
    }
    printf("GB2UniCode len:%x\n", len);
    *pDst = 0;
    return len;
}

int GB2UniCodeEnCn(BYTE *srcStr, WORD *pDst, int num)
{
    int     i, len = 0;
    WORD    uCode;
    BYTE *pDstB;
    PRINTF_FUN_LINE

    for (i = 0; len < num; ++i) {
        if (srcStr[i] == 0)
            break;
        if (srcStr[i] & 0x80 && (srcStr[i + 1] & 0x80)) {
            uCode = GB2UniCodeOne(srcStr[i], srcStr[i + 1]);
            ++i;
        } else {
            uCode = SwapWORD((WORD)srcStr[i]);
        }
        *pDst++ = uCode;
        len += 2;
        printf("GB2UniCodeEnCn uCode:%x\n", SwapWORD(uCode));
    }
    printf("GB2UniCodeEnCn len:%x\n", len);
    *pDst = 0;
    return len;
}

int GSM_Byte2Str(unsigned char *from, char *str, int len)
{
    int   i = 0;
    unsigned char  *pSrc;
    PRINTF_FUN_LINE

    pSrc = from;

    while (len--) {
        str[i++] = HEXCH[(*pSrc >> 4) & 0xF];
        str[i++] = HEXCH[*pSrc & 0xF];

        ++pSrc;
    }
    str[i] = 0;

    return  i;
}

static int sms_pdu_encode(gsm_sms_t *pSms, raw_sms_t *pdu)
{
    static unsigned char    tmpBuf[MAX_SMS_LENGTH + 40];
    unsigned char    *p;
    int        len, sclen;
    PRINTF_FUN_LINE

    if (*(pSms->address) == 0) {
        return -1;
    }

#if 1
    if (*gSmsCenter == 0) {
        p = tmpBuf;
        *p++ = 0x0;                    /* no center */
        sclen = 1;
    } else {
        strncpy(pSms->center, gSmsCenter, MAX_PHONE_NUMBER);
        pdu->sms = pSms;

        p = tmpBuf;

        len = sms_code_number(pSms->center, p + 1);
        *p++ = (unsigned char)len;
        p   += len;
        sclen = (p - tmpBuf);
    }
#endif

    *p++ = 0x11;
    *p++ = 0x0;
    *p++ = 0x0D;

    len = (char)strlen(pSms->address);
    if (*pSms->address == '+')
        len--;
    len = sms_code_number(pSms->address, p);
    p += len;

    *p++ = 0x0;
    *p++ = 0x08;
    *p++ = 0x0;

    //len = GB2UniCode((unsigned char*)pSms->msg, (WORD*)pdu->pdu, SMS_HZ_SIZE);
    len = GB2UniCodeEnCn((unsigned char*)pSms->msg, (WORD*)pdu->pdu, SMS_HZ_SIZE);

    *p++ = (unsigned char)len;
    memcpy(p, pdu->pdu, len);
    p += len;
    *p   = 0;
    len = p - tmpBuf;
    sclen = len - sclen;

    len = GSM_Byte2Str(tmpBuf, pdu->pdu, len);

    pdu->pdu[len++] = 0x1A;
    pdu->pdu[len] = 0;
    return sclen;
}

int AT_SendSms(gsm_sms_t *msg, unsigned char eSmsType, char* strcmd_len, char* strcmd_pdu)
{
    void    *rv;
    int        len;
    PRINTF_FUN_LINE

    memset(&raw_pdu, 0, sizeof(raw_sms_t));
    raw_pdu.handfree = eSmsType;
    len = sms_pdu_encode(msg, &raw_pdu);
    sprintf(strcmd_len, "AT+CMGS=%d", len);
    if (len < 0) {
        return -1;
    }
    sprintf(strcmd_pdu, "%s", raw_pdu.pdu);
    return 0;
}

int AT_SmsSendTo(char *who, char *msg, unsigned char eSmsType, char* strcmd_len, char* strcmd_pdu)
{
    int  ret;
    gsm_sms_t    sms;

    PRINTF_FUN_LINE

    if ((who == NULL) || (msg == NULL) || (*who == 0) || (msg[0] == 0))
        return -1;
    memset(&sms, 0, sizeof(gsm_sms_t));
    strncpy(sms.address, who, MAX_PHONE_NUMBER - 1);
    strncpy(sms.msg, msg, MAX_SMS_LENGTH);
    ret = AT_SendSms(&sms, eSmsType, strcmd_len, strcmd_pdu);
    return ret;
}

static int sms_pdu_decode_number(unsigned char *pSrc, char *pDst, int num, int type)
{
    int     pos;
    unsigned char tmp;
    unsigned char *p;

    PRINTF_FUN_LINE

    p = pSrc;
    pos = num;

    num = num << 1;

    if (type == 1) //tell
        num--;

    if (type != 2) {
        while (pos--) {
            tmp = *(p + 1);
            *(p + 1) = *p;
            *p = tmp;
            p += 2;
        }
    }

    memcpy(pDst, pSrc, num);
    pDst[num] = 0;

    return pos;
}

WORD UniCode2GBOne(WORD page)
{
    int i;
    WORD nRet;

    PRINTF_FUN_LINE

    for (i = 0; i < TABLE_NUM; i++) {
        if (page == GB2UC[i][1]) {
            nRet = (WORD)GB2UC[i][0];
        }
    }

    return nRet;
}

int UniCode2GBEnCn(BYTE *srcStr, BYTE *pDst, int num)
{
    int     i, len = 0;
    WORD    uCode;
    BYTE    pDst1;
    BYTE    pDst2;

    PRINTF_FUN_LINE

    for (i = 0; i < num; i += 4) {
        pDst1 = (CHAR2HEX(srcStr[i]) << 4) + CHAR2HEX(srcStr[i + 1]);
        pDst2 = (CHAR2HEX(srcStr[i + 2]) << 4) + CHAR2HEX(srcStr[i + 3]);
        uCode = (pDst1 << 8) + pDst2;
        uCode = UniCode2GBOne(uCode);
        pDst[len] = ((uCode >> 8) & 0xFF);
        pDst[len + 1] = (uCode & 0xFF);
        len += 2;
    }
    pDst[len] = 0;

    return len;
}

static int sms_pdu_decode(gsm_sms_t *pSms, char *raw_pdu)
{
    unsigned char len;
    int         pdulen;
    unsigned char   *p;
    char        tempbuff[20];

    p = raw_pdu;
    //get center
    p += 6; //unknow num
    sms_pdu_decode_number(p, pSms->center, 6, 1);
    p += 12;
    printf("sms_pdu_decode readmsg center= %s\n", pSms->center);

    p += 6; //unknow num
    //get send tell num
    sms_pdu_decode_number(p, pSms->address, 6, 1);
    printf("sms_pdu_decode readmsg address= %s\n", pSms->address);
    p += 12;

    p += 4; //unknow num
    //get date & time
    sms_pdu_decode_number(p, tempbuff, 3, 0);
    p += 6;
    pSms->date = atoi(tempbuff);
    printf("sms_pdu_decode readmsg date= %6d\n", pSms->date);

    //get date & time
    sms_pdu_decode_number(p, tempbuff, 3, 0);
    p += 8;
    pSms->time = atoi(tempbuff);
    printf("sms_pdu_decode readmsg time= %6d\n", pSms->time);

    //getmsg
    sms_pdu_decode_number(p, tempbuff, 1, 2);
    p += 2;
    pSms->length = atoi(tempbuff);
    pSms->length = pSms->length << 1;
    printf("sms_pdu_decode readmsg length= %d\n", pSms->length);

    //
    UniCode2GBEnCn((BYTE *)p, pSms->msg, pSms->length);
    printf("vicent readmsg msg= %s\n", pSms->msg);

    return 0;
}

int at_sms_read(char *raw_pdu, char* msg_buf)
{
    gsm_sms_t sms;
    sms_pdu_decode(&sms, raw_pdu);
    memcpy(msg_buf, sms.msg, (strlen(sms.msg) + 1));
}

