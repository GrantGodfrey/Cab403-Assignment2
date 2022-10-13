#pragma once
#include <pthread.h>

typedef struct entrance
{
    struct lpr license_plate_recognition;
    struct boom_gate;
    struct parkingsign;
} shared_entrance_t;

typedef struct exit
{
    struct lpr license_plate_recognition;
    struct boom_gate;
} shared_exit_t;

typedef struct level
{
    struct lpr license_plate_recognition;
    int temperature;
    int alarm_active;
} shared_level_t;

typedef struct lpr
{
    pthread_mutex_t lock;
    pthread_cond_t cond;
    char license_plate;
} shared_lpr_t;

typedef struct PARKING
{
    entrance entrances;
    exit exits;
    level levels;
};

typedef struct shared_memory
{
    const char *name;
    int fd;
    PARKING parking;
} shared_memory_t;
