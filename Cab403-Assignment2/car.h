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
    entrance entrences;
    exit exits;
    level levels;
} shared_parking_t;

typedef struct shm
{
    const char *name;
    int fd;
    PARKING parking;
} shm_t;
