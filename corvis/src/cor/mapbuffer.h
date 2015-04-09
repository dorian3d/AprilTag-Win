// Created by Eagle Jones
// Copyright (c) 2013, RealityCap, Inc.
// All Rights Reserved.

#ifndef __MAPBUFFER_H
#define __MAPBUFFER_H

#include <sys/types.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdint.h>
#include "cor.h"

struct mapbuffer {
    unsigned char *buffer;
    dispatch_t *dispatch;
    packet_t *last_packet;
    size_t size;
    size_t free_ptr;
    size_t waiting_on;
    size_t bytes_left;
    uint64_t total_bytes;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int fd;
    bool replay;
    bool block_when_full;
    bool has_blocked;
    const char *filename;
    char shm_filename[13];
    int shm_fd;
};

void mapbuffer_copy_packet(struct mapbuffer *mb, packet_t *p);
packet_t *mapbuffer_alloc(struct mapbuffer *mb, enum packet_type type, uint32_t bytes);
void mapbuffer_enqueue(struct mapbuffer *mb, packet_t *p, uint64_t time);
struct plugin mapbuffer_open(struct mapbuffer *mb);
struct plugin mapbuffer_open_with_path(struct mapbuffer *mb, const char * path);
void mapbuffer_close(struct mapbuffer *mb);
packet_t *mapbuffer_read(struct mapbuffer *mb, uint64_t *offset);

#endif
