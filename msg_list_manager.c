/**
 * Copyright (C) 2017 Fuzhou Rockchip Electronics Co., Ltd
 * Author: Chad.ma <chad.ma@rock-chips.com>
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "common.h"
#include "msg_list_manager.h"

static struct type_node *g_listHead = NULL;

struct type_node *createList(void)
{
    struct type_node *l = (struct type_node *)malloc(sizeof(struct type_node));

    if (NULL == l)
        return NULL;

    l->callback = NULL;
    l->next = NULL;

    return l;
}

static struct type_node *appendList(struct type_node *l, fun_cb cb)
{
    struct type_node *p = NULL;

    if (NULL == l)
        return NULL;

    p = (struct type_node *)malloc(sizeof(struct type_node));

    if (NULL == p)
        return NULL;

    memset(p, 0, sizeof(struct type_node));

    /* Fisrt node */
    if (l->next == NULL) {
        p->callback = cb;
        p->next = NULL;
        l->next = p;
        return p;
    } else {
        while (l->next != NULL)
            l = l->next;
    }

    p->callback = cb;
    p->next = NULL;
    l->next = p;

    return p;
}

static int deleteNode(struct type_node *node)
{
    struct type_node *p = NULL;
    struct type_node *pre = NULL;

    if (NULL == node)
        return 0;

    p = g_listHead;

    while (p != node) {
        pre = p;
        p = p->next;
    }

    pre->next = p->next;
    free(p);

    printf("deleteNode OK! \n");
    return 1;
}

static void destoryList(struct type_node *l)
{
    struct type_node *next;
    struct type_node *p = l;

    if (NULL == p)
        return;

    while (p != NULL) {
        next = p->next;
        free(p);
        p = next;
    }
}

struct type_node *init_list(void)
{
    g_listHead = createList();
    return g_listHead;
}

struct type_node *reg_entry(fun_cb cb)
{
    return appendList(g_listHead, cb);
}

int unreg_entry(fun_cb cb)
{
    struct type_node *node = NULL;

    if (cb != NULL) {
        node = g_listHead;
        while(node != NULL && node->callback != cb)
            node = node->next;
    }

    return deleteNode(node);
}

void deinit_list(void)
{
    if (NULL != g_listHead) {
        destoryList(g_listHead);
        g_listHead = NULL;
    }
}

void dispatch_msg(void *msg, void *prama0, void *param1)
{
    struct type_node *p = g_listHead;

    if (!p) {
        printf("list is NULL \n");
        return;
    }
    do {
        if (p->callback)
            p->callback(msg, prama0, param1);
        p = p->next;
    } while (p);
}
