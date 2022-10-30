#include <semaphore.h>
#include <stdbool.h>
#pragma once

#define ENTRANCES 5
#define EXITS 5
#define LEVELS 5
#define MAX_CAPACITY 100
#define STORAGE_CAPACITY 7

/**
 * A car control structure.
 */
typedef struct car
{
    //liscense plate number
    char plate[STORAGE_CAPACITY];
    //level the car park into
    int level;
    //
    bool withdrawn;
    //the time start for car's parking 
    clock_t start;
    //the time finish for car's parking
    clock_t finish;
    //
    int count;
} car_t;

/**
 * A car memory control structure.
 */
typedef struct carMemory
{
    //The current number of cars in the car memory
    int size;
    //
    car_t car[MAX_CAPACITY];
} carMemory_t;

/**
 * A car vector control structure.
 */
typedef struct carVector
{
    //The current number of elements in the car vector
    int size;
    //liscense plate queue
    char plateQueue[MAX_CAPACITY][STORAGE_CAPACITY];
} carVector_t;

/**
 * A license plate reader control structure.
 */
typedef struct licensePlateReader
{
    pthread_mutex_t m;
    pthread_cond_t c;
    //the amout of character in a license plate
    char plate[7];
} licensePlateReader_t;

/**
 * A boom gate control structure.
 */
typedef struct boomGate
{
    pthread_mutex_t m;
    pthread_cond_t c;
    //the status of a boom gate
    char status;
} boomGate_t;

/**
 * An information sign board control structure.
 */
typedef struct informationSign
{
    pthread_mutex_t m;
    pthread_cond_t c;
    //the displied information
    char display;
} informationSign_t;

/**
 * An entrance situation control structure.
 */
typedef struct entrance
{
    //license plate reader and its condition
    licensePlateReader_t licensePlateReader;
    //boomgate status and its condition
    boomGate_t boomGate;
    //information sign with displaied information
    informationSign_t informationSign;
} entrance_t;

/**
 * An exit situation control structure.
 */
typedef struct exit
{
    //license plate reader and its condition
    licensePlateReader_t licensePlateReader;
    //boomgate status and its condition
    boomGate_t boomGate;
} exit_t;

/**
 * A level control structure.
 */
typedef struct level
{
    //license plate reader and its condition
    licensePlateReader_t licensePlateReader;
    //the temperature of respective level
    volatile int16_t tempSensor;
    //fire alarm situation of respective level
    volatile char fireAlarm;
} level_t;

/**
 * A shared data block.
 */
typedef struct shared_data 
{
    entrance_t entrance[ENTRANCES];

    exit_t exit[EXITS];

    level_t level[LEVELS];
}shared_data_t;

/**
 * A shared memory control structure.
 */
typedef struct shared_memory
{
    // The name of the shared memory object.
    const char *name;
    // The file descriptor used to manage the shared memory object.
    int fd;
    // Address of the shared data block. 
    shared_data_t* data;
} shared_memory_t;

bool readAndWriteObject(shared_memory_t *shm, const char *share_name);
bool readObject(shared_memory_t *shm, const char *share_name);
void initialiseSharedMemory(shared_memory_t shm);
void addLP(carVector_t *carVector, char *plate);
void popLP(carVector_t *carVector);
void plateInit(carVector_t *carVector);