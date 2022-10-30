// -------------------------------------------- HEADER -------------------------------------------- //
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

char approved_plates[100][7];
shared_memory_t shm;
carVector_t entranceQueue[ENTRANCES];
pthread_mutex_t entranceQueueMutex[ENTRANCES];
int selector;

void addLP(carVector_t *carVector, char *plate)
{
    strcpy(carVector->plateQueue[carVector->size], plate);
    carVector->size = carVector->size + 1;
}

void popLP(carVector_t *carVector)
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

char *randomPlate()
{
    int number[3] = {};
    for (int i = 0; i < 3; i++)
    {
        number[i] = generateRandom(0, 9);
    }

    char character[3] = {};
    for (int i = 0; i < 3; i++)
    {
        character[i] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"[generateRandom(0, 25)];
    }

    char *finstr = NULL;
    finstr = malloc(10);
    for (int i = 0; i < 3; i++)
    {
        sprintf(&finstr[i], "%d", number[i]);
    }

    finstr[3] = character[0];
    finstr[4] = character[1];
    finstr[5] = character[2];
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


void *spawnCar(void *args)
{
    char *plate;
    int waitTime;
    
    for (int i = 0; i < ENTRANCES; i++)
    {
        // plateInit(&entranceQueue[i]);

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
        printf("The plate %s is arriving at entrance %d\n", plate, entranceCar + 1);
        pthread_mutex_lock(&entranceQueueMutex[entranceCar]);
        addLP(&entranceQueue[entranceCar], plate);
        pthread_mutex_unlock(&entranceQueueMutex[entranceCar]);
    }
    return 0;
}

void *entranceSimulate(void *arg) 
{
    int i = *(int *)arg;
    for (;;)
    {
        pthread_mutex_lock(&shm.data->entrance[i].licensePlateReader.m);
        while (strcmp(shm.data->entrance[i].licensePlateReader.plate, startLPR))
        {
            pthread_cond_wait(&shm.data->entrance[i].licensePlateReader.c, &shm.data->entrance[i].licensePlateReader.m);
        }
        pthread_mutex_unlock(&shm.data->entrance[i].licensePlateReader.m);
        while (entranceQueue[i].size <= 0)
            ;
        pthread_mutex_lock(&shm.data->entrance[i].licensePlateReader.m);
        pthread_mutex_lock(&entranceQueueMutex[i]);
        strcpy(shm.data->entrance[i].licensePlateReader.plate, entranceQueue[i].plateQueue[0]);
        pthread_mutex_unlock(&entranceQueueMutex[i]);
        pthread_mutex_unlock(&shm.data->entrance[i].licensePlateReader.m);
        usleep(2000);
        pthread_cond_signal(&shm.data->entrance[i].licensePlateReader.c);
        pthread_mutex_lock(&entranceQueueMutex[i]);
        popLP(&entranceQueue[i]);
        pthread_mutex_unlock(&entranceQueueMutex[i]);
    }
}

int generateRandom(int lower, int upper)
{
    int num = (rand() % (upper - lower + 1)) + lower;
    return num;
}

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

void *tempSensorSimulate(void *arg) 
{
    int i = *(int *)arg;
    int16_t temp;
    int16_t currentTemp;

    do
    {
        usleep(2000);
        if (FIRE == 2)
        { 

            if (shm.data->level[i].tempSensor > 58)
            {
                currentTemp = 24;
            }
            else
            {
                currentTemp = shm.data->level[i].tempSensor;
            }
            temp = generateRandom(currentTemp, currentTemp + 2);
            shm.data->level[i].tempSensor = temp;
        }
        else if (FIRE == 1)
        { 
            temp = (int16_t)generateRandom(58, 68);
            shm.data->level[i].tempSensor = temp;
        }
        
        else
        {
            temp = (int16_t)24;
            shm.data->level[i].tempSensor = temp;
        }
    }while(1);
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
    time_t t;
    srand((unsigned)time(&t));
    create_shared_object_RW(&shm, SHARE_NAME);

    for (int i = 0; i < ENTRANCES; i++)
    {
        shm.data->entrance[i].boomGate.status = 'C';
        strcpy(shm.data->entrance[i].licensePlateReader.plate, startLPR);
    }

    for (int i = 0; i < ENTRANCES; i++)
    {
        pthread_mutex_init(&shm.data->entrance[i].licensePlateReader.m, &am);
        pthread_cond_init(&shm.data->entrance[i].licensePlateReader.c, &attr_c);
    }
    for (int i = 0; i < ENTRANCES; i++)
    {
        pthread_mutex_init(&shm.data->entrance[i].boomGate.m, &am);
        pthread_cond_init(&shm.data->entrance[i].boomGate.c, &attr_c);
    }
    for (int i = 0; i < ENTRANCES; i++)
    {
        pthread_mutex_init(&shm.data->entrance[i].informationSign.m, &am);
        pthread_cond_init(&shm.data->entrance[i].informationSign.c, &attr_c);
    }

    for (int i = 0; i < EXITS; i++)
    {
        shm.data->exit[i].boomGate.status = 'C';
        strcpy(shm.data->exit[i].licensePlateReader.plate, startLPR);
    }

    for (int i = 0; i < EXITS; i++)
    {
        pthread_mutex_init(&shm.data->exit[i].boomGate.m, &am);
        pthread_cond_init(&shm.data->exit[i].boomGate.c, &attr_c);

    }

    for (int i = 0; i < EXITS; i++)
    {
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
        pthread_mutex_init(&shm.data->level[i].licensePlateReader.m, &am);
        pthread_cond_init(&shm.data->level[i].licensePlateReader.c, &attr_c);   
    }

    for (int i = 0; i <LEVELS; i++)
    {
        strcpy(shm.data->level[i].licensePlateReader.plate, startLPR);
    }

    readFile("plates.txt");

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