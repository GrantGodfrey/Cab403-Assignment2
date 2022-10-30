#include <semaphore.h>
#include <stdbool.h>
#pragma once

#define ENTRANCES 5
#define EXITS 5
#define LEVELS 5
#define MAX_CAPACITY 100
#define STORAGE_CAPACITY 7

typedef struct car
{
    char plate[STORAGE_CAPACITY];
    int level;
    bool withdrawn;
    clock_t start;
    clock_t finish;
    int count;
} car_t;

typedef struct carMemory
{
    int size;
    car_t car[MAX_CAPACITY];
} carMemory_t;

typedef struct carVector
{
    int size;
    char plateQueue[MAX_CAPACITY][STORAGE_CAPACITY];
} carVector_t;

typedef struct licensePlateReader
{
    pthread_mutex_t m;
    pthread_cond_t c;
    char plate[7];
} licensePlateReader_t;

typedef struct boomGate
{
    pthread_mutex_t m;
    pthread_cond_t c;
    char status;
} boomGate_t;

typedef struct informationSign
{
    pthread_mutex_t m;
    pthread_cond_t c;
    char display;
} informationSign_t;

typedef struct entrance
{
    licensePlateReader_t licensePlateReader;
    boomGate_t boomGate;
    informationSign_t informationSign;
} entrance_t;

typedef struct exit
{
    licensePlateReader_t licensePlateReader;
    boomGate_t boomGate;
} exit_t;

typedef struct level
{
    licensePlateReader_t licensePlateReader;
    volatile int16_t tempSensor;
    volatile char fireAlarm;
} level_t;

typedef struct shared_data 
{
    entrance_t entrance[ENTRANCES];

    exit_t exit[EXITS];

    level_t level[LEVELS];
}shared_data_t;

typedef struct shared_memory
{
    const char *name;
    int fd;
    shared_data_t* data;
} shared_memory_t;

bool create_shared_object_RW(shared_memory_t *shm, const char *share_name);
bool create_shared_object_R(shared_memory_t *shm, const char *share_name);
void initialiseSharedMemory(shared_memory_t shm);
void addLP(carVector_t *carVector, char *plate);
void popLP(carVector_t *carVector);
void plateInit(carVector_t *carVector);
