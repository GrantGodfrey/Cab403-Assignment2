#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <semaphore.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <string.h>

#include "car.h"

const int SIZE = 2920;
#define LEVELS 5
#define ENTRANCES 5
#define EXITS 5

bool create_shared_object(shared_memory_t *shm, const char *share_name)
{
    // Remove any previous instance of the shared memory object, if it exists.
    if (share_name > 0)
    {
        shm_unlink(share_name);
    }

    // Assign share name to shm->name.
    shm->name = share_name;

    // Create the shared memory object, allowing read-write access, and saving the
    // resulting file descriptor in shm->fd.
    shm->fd = shm_open(share_name, O_CREAT | O_RDWR, 0666);
    if (shm->fd == -1)
    {
        shm = NULL;
        return false;
    }

    // Set the capacity of the shared memory object via ftruncate.
    ftruncate(shm->fd, sizeof(shared_parking_t));
    if ((ftruncate(shm->fd, sizeof(shared_parking_t))) == -1)
    {
        shm = NULL;
        return false;
    }

    // Attempt to map the shared memory via mmap, and save the address
    // in shm->data.
    shm = mmap(0, sizeof(shared_parking_t), PROT_WRITE, MAP_SHARED, shm->fd, 0);
    if (shm == MAP_FAILED)
    {
        return false;
    }

    return true;
}

void destroy_shared_object(shared_memory_t *shm)
{
    // Remove the shared memory object.
    munmap(shm, sizeof(shared_parking_t));
    shm_unlink(shm->name);
    shm = NULL;
    shm->fd = -1;
}

void sharedMemoryInitialisation(shared_memory_t shm)
{
    int i;

    pthread_mutexattr_t attr_mutex;
    pthread_condattr_t attr_condition;

    pthread_mutexattr_init(&attr_mutex);
    pthread_condattr_init(&attr_condition);
    pthread_mutexattr_setpshared(&attr_mutex, PTHREAD_PROCESS_SHARED);
    pthread_condattr_setpshared(&attr_condition, PTHREAD_PROCESS_SHARED);

    for (i = 0; i < ENTRANCES; i++)
    {
        // Mutex and Condition initialization

        // Entrance LPR sensors
        pthread_mutex_init(&shm.parking.entrances[i].license_plate_recognition.lock, &attr_mutex);
        pthread_cond_init(&shm.parking.entrances[i].license_plate_recognition.cond, &attr_condition);
        // Entrance Boomgates
        pthread_mutex_init(&shm.parking.entrances[i].boom_gate.m, &attr_mutex);
        pthread_cond_init(&shm.parking.entrances[i].boom_gate.c, &attr_condition);
        // Entrance Parking Signs
        pthread_mutex_init(&shm.parking.entrances[i].parkingsign.m, &attr_mutex);
        pthread_cond_init(&shm.parking.entrances[i].parkingsign.c, &attr_condition);

        // Update status of entrance boomgates to 'Closed'('C')
        shm.parking.entrances[i].boom_gate.s = 'C';
        strcpy(shm.parking.entrances[i].license_plate_recognition.license_plate, "xxxxxx");
    }

    for (int i = 0; i < EXITS; i++)
    {
        // Exit LPR sensors
        pthread_mutex_init(&shm.parking.exits[i].license_plate_recognition.lock, &attr_mutex);
        pthread_cond_init(&shm.parking.exits[i].license_plate_recognition.cond, &attr_condition);
        // Exit Boomgates
        pthread_mutex_init(&shm.parking.exits[i].boom_gate.m, &attr_mutex);
        pthread_cond_init(&shm.parking.exits[i].boom_gate.c, &attr_condition);

        // Update status of exit boomgates to 'Closed'('C')
        shm.parking.exits[i].boom_gate.s = 'C';
        strcpy(shm.parking.exits[i].license_plate_recognition.license_plate, "xxxxxx");
    }

    for (int i = 0; i < LEVELS; i++)
    {
        // Level LPR sensors
        pthread_mutex_init(&shm.parking.levels[i].license_plate_recognition.lock, &attr_mutex);
        pthread_cond_init(&shm.parking.levels[i].license_plate_recognition.cond, &attr_condition);
        shm.parking.levels[i].alarm_active = 0;
        shm.parking.levels[i].temperature = 24;

        // Update license plate to default value of 'xxxxxx'
        strcpy(shm.parking.levels[i].license_plate_recognition.license_plate, "xxxxxx");
    }
}

int main()
{
}