#include <semaphore.h>
#include <sys/stat.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <stdbool.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>
#include <stdint.h>
#include <sys/types.h>

#include "sharedMemory.h"

#define SHARE_NAME "PARKING"
#define CAR_LIMIT 20
#define FIRE 0
#define RANDOM_CHANCE 80
#define MAX_CAPACITY 100
#define STORAGE_CAPACITY 7
char startLPR[7] = "000000";




bool create_shared_object_RW(shared_memory_t *shm, const char *share_name)
{

    shm_unlink(share_name);
    shm->name = share_name;

    if ((shm->fd = shm_open(share_name, O_CREAT | O_RDWR, 0666)) < 0)
    {
        shm->data = NULL;
        return false;
    }

    if (ftruncate(shm->fd, sizeof(shared_data_t)) == -1)
    {
        shm->data = NULL;
        return false;
    }

    if ((shm->data = mmap(0, sizeof(shared_data_t), PROT_WRITE, MAP_SHARED, shm->fd, 0)) == (void *)-1)
    {
        return false;
    }

    return true;
}

void destroy_shared_object(shared_memory_t *shm)
{
    munmap(shm, 48);
    shm->data = NULL;
    shm->fd = -1;
    shm_unlink(shm->name);
}


char approved_plates[100][7];
shared_memory_t shm;
carVector_t entranceQueue[ENTRANCES];
pthread_mutex_t entranceQueueMutex[ENTRANCES];
int selector;

void addPlate(carVector_t *carVector, char *plate)
{
    strcpy(carVector->plateQueue[carVector->size], plate);
    carVector->size = carVector->size + 1;
}


void popPlate(carVector_t *carVector)
{

    char old_data[MAX_CAPACITY][STORAGE_CAPACITY];
    for (int i = 0; i < carVector->size; i++)
    {
        strcpy(old_data[i], carVector->plateQueue[i]);
    }
    for (int i = 0; i < carVector->size - 1; i++)
    {
        strcpy(carVector->plateQueue[i], old_data[i + 1]);
    }
    carVector->size = carVector->size - 1;
}

void popRandom(carVector_t *carVector, int index)
{
    char old_data[MAX_CAPACITY][STORAGE_CAPACITY];
    for (int i = 0; i < carVector->size; i++)
    {
        strcpy(old_data[i], carVector->plateQueue[i]);
    }
    for (int i = 0; i < index; i++)
    {
        strcpy(carVector->plateQueue[i], old_data[i]);
    }

    for (int i = index; i < carVector->size - 1; i++)
    {
        strcpy(carVector->plateQueue[i], old_data[i + 1]);
    }
    carVector->size = carVector->size - 1;
}

char *randomPlate()
{
    int first = generateRandom(0, 9);
    int second = generateRandom(0, 9);
    int third = generateRandom(0, 9);

    char randomletter1 = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"[generateRandom(0, 25)];
    char randomletter2 = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"[generateRandom(0, 25)];
    char randomletter3 = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"[generateRandom(0, 25)];

    char *finstr = NULL;
    finstr = malloc(10);
    sprintf(&finstr[0], "%d", first);
    sprintf(&finstr[1], "%d", second);
    sprintf(&finstr[2], "%d", third);
    finstr[3] = randomletter1;
    finstr[4] = randomletter2;
    finstr[5] = randomletter3;
    finstr[6] = '\0';

    return finstr;
}

char *generatePlate(int probability)
{
    int random = generateRandom(0, 100);
    if (random <= probability)
    {
        return approved_plates[selector];
    }
    else
    {
        char *p = randomPlate();
        return p;
    }
}


void *spawnCar(void *args) // needs work
{
    char *plate;
    int waitTime;
    
    for (int i = 0; i < ENTRANCES; i++)
    {
        entranceQueue->size = 0;
        for (int i = 0; i < MAX_CAPACITY; i++)
        {
            strcpy(entranceQueue->plateQueue[i], "empty");
        }
    }

    for (int i = 0; i < CAR_LIMIT; i++)
    {
        plate = generatePlate(RANDOM_CHANCE);
        selector++;

        waitTime = generateRandom(1, 100) * 1000;
        usleep(waitTime);

        int entranceCar = generateRandom(0, ENTRANCES - 1);

        printf("%s has arrived at entrance %d\n", plate, entranceCar + 1);
        pthread_mutex_lock(&entranceQueueMutex[entranceCar]);
        addPlate(&entranceQueue[entranceCar], plate);
        pthread_mutex_unlock(&entranceQueueMutex[entranceCar]);
    }
    return 0;
}

void *entranceSimulate(void *arg) //needs change
{
    int i = *(int *)arg;

    do
    {
        pthread_mutex_lock(&shm.data->entrance[i].licensePlateReader.m);
        while (strcmp(shm.data->entrance[i].licensePlateReader.plate, startLPR))
        {
            pthread_cond_wait(&shm.data->entrance[i].licensePlateReader.c, &shm.data->entrance[i].licensePlateReader.m);
        }
        pthread_mutex_unlock(&shm.data->entrance[i].licensePlateReader.m);
        while (entranceQueue[i].size <= 0);

        pthread_mutex_lock(&shm.data->entrance[i].licensePlateReader.m);
        pthread_mutex_lock(&entranceQueueMutex[i]);
        strcpy(shm.data->entrance[i].licensePlateReader.plate, entranceQueue[i].plateQueue[0]);
        pthread_mutex_unlock(&entranceQueueMutex[i]);
        pthread_mutex_unlock(&shm.data->entrance[i].licensePlateReader.m);

        usleep(2000);

        pthread_cond_signal(&shm.data->entrance[i].licensePlateReader.c);
        pthread_mutex_lock(&entranceQueueMutex[i]);
        popPlate(&entranceQueue[i]);
        pthread_mutex_unlock(&entranceQueueMutex[i]);
    }while(1);
}

int generateRandom(int lower, int upper)
{
    int num = (rand() % (upper - lower + 1)) + lower;
    return num;
}

// Reads contents of supplied file
void readFile(char *name)
{
    FILE *file = fopen(name, "r");
    int i = 0;
    while (fgets(approved_plates[i], 10, file))
    {
        approved_plates[i][strlen(approved_plates[i]) - 1] = '\0';
        i++;
    }
}

// Prints contents of numberplate file (USED FOR TESTING)
void printFile()
{
    printf("\n The content of the file  are : \n");
    for (int i = 0; i < 100; i++)
    {
        printf("%s, ", approved_plates[i]);
    }
    printf("\n");
}

// Simulates temperatures
void *tempSensorSimulate(void *arg) // needs change
{
    int i = *(int *)arg;
    int16_t temperature;
    int16_t currentTemp;

    for (;;)
    {
        usleep(2000);
        if (FIRE == 1)
        { // (Fixed temp fire detection data)
            // Generate temperatures to trigger fire alarm via Temps > 58 degrees
            temperature = (int16_t)generateRandom(58, 65);
            shm.data->level[i].tempSensor = temperature;
        }
        else if (FIRE == 2)
        { // (Rate-of-rise fire detection data)
            // Generate temperatures to trigger fire alarm via Rate-of-rise (Most recent temp >= 8 degrees hotter than 30th most recent)
            if (shm.data->level[i].tempSensor > 58)
            {
                currentTemp = 24;
            }
            else
            {
                currentTemp = shm.data->level[i].tempSensor;
            }
            temperature = generateRandom(currentTemp, currentTemp + 2);
            shm.data->level[i].tempSensor = temperature;
        }
        else
        {
            // Generate normal temperatures to avoid setting off fire alarm
            temperature = (int16_t)24;
            shm.data->level[i].tempSensor = temperature;
        }
    }
}
int main()
{
    selector = 0;
    pthread_mutexattr_t am;
    pthread_condattr_t attr_c;
    pthread_t entranceThread[ENTRANCES];
    pthread_t tempSensorSimulate_thread[LEVELS];
    pthread_t carSpawner;
    pthread_mutexattr_init(&am);
    pthread_condattr_init(&attr_c);
    pthread_mutexattr_setpshared(&am, PTHREAD_PROCESS_SHARED);
    pthread_condattr_setpshared(&attr_c, PTHREAD_PROCESS_SHARED);

    for (int i = 0; i < ENTRANCES; i++)
    {
        pthread_mutex_init(&entranceQueueMutex[ENTRANCES], NULL);
    }

    // Initialise random seed
    time_t t;
    srand((unsigned)time(&t));

    // Create shared memory object
    create_shared_object_RW(&shm, SHARE_NAME);

    for (int i = 0; i < ENTRANCES; i++)
    {
         // Initiliase status of the gates to 'closed'
        shm.data->entrance[i].boomGate.status = 'C';
        strcpy(shm.data->entrance[i].licensePlateReader.plate, startLPR);
    }

    // Initialise mutexs and condition variables in shared memory
    for (int i = 0; i < ENTRANCES; i++)
    {
        // Intilise Mutexs and Condition variables for LPR sensors
        pthread_mutex_init(&shm.data->entrance[i].licensePlateReader.m, &am);
        pthread_cond_init(&shm.data->entrance[i].licensePlateReader.c, &attr_c);
    }
    for (int i = 0; i < ENTRANCES; i++)
    {
         // Intilise Mutexs and Condition variables for Gates
        pthread_mutex_init(&shm.data->entrance[i].boomGate.m, &am);
        pthread_cond_init(&shm.data->entrance[i].boomGate.c, &attr_c);

    }
    for (int i = 0; i < ENTRANCES; i++)
    {
        
        // Intilise Mutexs and Condition variables for Information signs
        pthread_mutex_init(&shm.data->entrance[i].informationSign.m, &am);
        pthread_cond_init(&shm.data->entrance[i].informationSign.c, &attr_c);
    }

    for (int i = 0; i < EXITS; i++)
    {
        // Initiliase status of the gates to 'closed'
        shm.data->exit[i].boomGate.status = 'C';
        strcpy(shm.data->exit[i].licensePlateReader.plate, startLPR);
    }

    for (int i = 0; i < EXITS; i++)
    {
        // Intilise Mutexs and Condition variables for Gates
        pthread_mutex_init(&shm.data->exit[i].boomGate.m, &am);
        pthread_cond_init(&shm.data->exit[i].boomGate.c, &attr_c);

    }

    for (int i = 0; i < EXITS; i++)
    {
        // Intilise Mutexs and Condition variables for LPR sensors
        pthread_mutex_init(&shm.data->exit[i].licensePlateReader.m, &am);
        pthread_cond_init(&shm.data->exit[i].licensePlateReader.c, &attr_c);
    }

    for (int i = 0; i < LEVELS; i++)
    {
        shm.data->level[i].fireAlarm = '0';
        shm.data->level[i].tempSensor = 24;
    }

    for (int i = 0; i < LEVELS; i++)
    {
        // Intilise Mutexs and Condition variables for LPR sensors
        pthread_mutex_init(&shm.data->level[i].licensePlateReader.m, &am);
        pthread_cond_init(&shm.data->level[i].licensePlateReader.c, &attr_c);   
    }

    for (int i = 0; i <LEVELS; i++)
    {
         // Initiliase number plate to be xxxxxx
        strcpy(shm.data->level[i].licensePlateReader.plate, startLPR);
    }

    // Read the number plates
    readFile("plates.txt");

    // Create threads
    int i;

    pthread_create(&carSpawner, NULL, &spawnCar, NULL);
    for (i = 0; i < ENTRANCES; i++)
    {
        int *p = malloc(sizeof(int));
        *p = i;
        pthread_create(&entranceThread[i], NULL, &entranceSimulate, p);
    }

    for (i = 0; i < LEVELS; i++)
    {
        int *z = malloc(sizeof(int));
        *z = i;
        pthread_create(&tempSensorSimulate_thread[i], NULL, &tempSensorSimulate, z);
    }

    // Join threads
    pthread_join(carSpawner, NULL);

    for (i = 0; i < ENTRANCES; i++)
    {
        pthread_join(entranceThread[i], NULL);
    }
    for (i = 0; i < LEVELS; i++)
    {
        pthread_join(tempSensorSimulate_thread[i], NULL);
    }
}