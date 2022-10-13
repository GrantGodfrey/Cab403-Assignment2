#pragma once
#include <pthread.h>

#define ENTRANCES 5
#define EXITS 5
#define LEVELS 5

typedef struct boomgate
{
    pthread_mutex_t m;
    pthread_cond_t c;
    char s;
} shared_boomgate_t;

typedef struct parkingsign
{
    pthread_mutex_t m;
    pthread_cond_t c;
    char display;
} shared_parkingsign_t;

typedef struct lpr
{
    pthread_mutex_t lock;
    pthread_cond_t cond;
    char license_plate[7];
} shared_lpr_t;

typedef struct entrance
{
    shared_lpr_t license_plate_recognition;
    shared_boomgate_t boom_gate;
    shared_parkingsign_t parkingsign;
} shared_entrance_t;

typedef struct exit
{
    shared_lpr_t license_plate_recognition;
    shared_boomgate_t boom_gate;
} shared_exit_t;

typedef struct level
{
    shared_lpr_t license_plate_recognition;
    volatile int16_t temperature;
    volatile int alarm_active;
} shared_level_t;

typedef struct PARKING
{
    shared_entrance_t entrances[ENTRANCES];
    shared_exit_t exits[EXITS];
    shared_level_t levels[LEVELS];
} shared_parking_t;

typedef struct shared_memory
{
    const char *name;
    int fd;
    shared_parking_t parking;
} shared_memory_t;
