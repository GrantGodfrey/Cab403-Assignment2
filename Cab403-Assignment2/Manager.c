// -------------------------------------------- HEADER -------------------------------------------- //
#include "sharedMemory.h"
#include <semaphore.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>

#define SHARE_NAME "PARKING"
#define CAPACITY 1
#define ARSIZE 30

shared_memory_t shm;
carVector_t levelQueue[LEVELS];
carVector_t exitQueue[EXITS];
carMemory_t carMemory;

pthread_mutex_t levelQueueMutex[LEVELS];
pthread_mutex_t exitQueueMutex[EXITS];
pthread_mutex_t carMemoryM;
pthread_mutex_t levelCapacityMutex[LEVELS];

pthread_t lprThreadEntrance[ENTRANCES];
pthread_t lprThreadExit[EXITS];
pthread_t lprThreadLevel[LEVELS];
pthread_t lprThreadLevelCont[LEVELS];
pthread_t lprThreadExitCont[EXITS];
pthread_t statDisplayThread;
pthread_t timeCheckThread[LEVELS];
pthread_t boomgateThreadExit[EXITS];
pthread_t boomgateThreadEntrance[ENTRANCES];

// --------------------------------------- PUBLIC VARIABLES --------------------------------------- //
int levelCapacity[LEVELS];
char approved_plates[100][7];
char statusDisp[ENTRANCES];
char exitDisplay[EXITS];
char entrDisplay[ENTRANCES];
double bill;
int16_t rawData[LEVELS][ARSIZE] = {0};
double moneyEarned;
int fire;
char startLPR[7] = "000000";

// storage initializing of care park
void storageInit(carMemory_t *carMemory)
{
    carMemory->size = 0;
    for (int i = 0; i < MAX_CAPACITY; i++)
    {
        strcpy(carMemory->car[i].plate, startLPR);
        carMemory->car[i].start = 0;
        carMemory->car[i].finish = 0;
        carMemory->car[i].withdrawn = false;
        carMemory->car[i].level = 1;
        carMemory->car[i].count = 0;
    }
}

//adding cars into parking by level
void addCar(carMemory_t *carMemory, char *plate, clock_t start, clock_t finish, int level)
{
    
    int old_size = carMemory->size;
    memcpy(carMemory->car[old_size].plate, plate, 7);
    carMemory->car[old_size].start = start;
    carMemory->car[old_size].finish = finish;
    carMemory->car[old_size].level = level;
    carMemory->car[old_size].withdrawn = false;
    carMemory->car[old_size].count = 0;
    carMemory->size = old_size + 1;
}

//removing cars from parking 
void removeCar(carMemory_t *carMemory, char *plate)
{
    
    int old_size = carMemory->size;
    car_t old_car[MAX_CAPACITY];
    int loc;

    for (int i = 0; i < old_size; i++)
    {
        memcpy(old_car[i].plate, carMemory->car[i].plate, 7);
        old_car[i].start = carMemory->car[i].start;
        old_car[i].finish = carMemory->car[i].finish;
        old_car[i].level = carMemory->car[i].level;
        old_car[i].withdrawn = carMemory->car[i].withdrawn;
        old_car[i].count = carMemory->car[i].count;
    }

    for (int i = 0; i < old_size; i++)
    {
        if (memcmp(carMemory->car[i].plate, plate, 7) == 0)
        {
            loc = i;
            break;
        }
    }

    for (int i = 0; i < loc; i++)
    {
        memcpy(carMemory->car[i].plate, old_car[i].plate, 7);
        carMemory->car[i].start = old_car[i].start;
        carMemory->car[i].finish = old_car[i].finish;
        carMemory->car[i].level = old_car[i].level;
        carMemory->car[i].withdrawn = old_car[i].withdrawn;
        carMemory->car[i].count = old_car[i].count;
    }

    for (int i = loc; i < old_size - 1; i++)
    {
        memcpy(carMemory->car[i].plate, old_car[i + 1].plate, 7);
        carMemory->car[i].start = old_car[i + 1].start;
        carMemory->car[i].finish = old_car[i + 1].finish;
        carMemory->car[i].level = old_car[i + 1].level;
        carMemory->car[i].withdrawn = old_car[i + 1].withdrawn;
        carMemory->car[i].count = old_car[i + 1].count;
    }

    carMemory->size = old_size - 1;
}

//finding car according the licensePlate
int findIndex(carMemory_t *carMemory, char *plate)
{
    int index;
    for (int i = 0; i < carMemory->size; i++)
    {
        if (memcmp(carMemory->car[i].plate, plate, 7) == 0)
        {
            index = i;
            return index;
        }
    }
    return 1;
}

//printing the information of parked car
void printCarList(carMemory_t *carMemory)
{
    printf("NUMBER OF PLATES ARE: %d\n", carMemory->size);
    for (int i = 0; i < carMemory->size; i++)
    {
        printf("Plate number %d is: %s\n", i, carMemory->car[i].plate);
        printf("Entrance time of car %d is: %ld\n", i, carMemory->car[i].start);
        printf("Park time of car %d is: %ld\n", i, carMemory->car[i].finish);
        printf("Level of car %d is: %d\n", i, carMemory->car[i].level);
        printf("Number of LRP count %d is: %d\n", i, carMemory->car[i].count);
    }
}

// --------------------------------------------- MAIN --------------------------------------------- //
bool create_shared_object_R(shared_memory_t *shm, const char *share_name)
{
    // Assign share name to shm->name.
    shm->name = share_name;

    // Create the shared memory object, allowing read-write access
    if ((shm->fd = shm_open(share_name, O_RDWR, 0)) < 0)
    {
        shm->data = NULL;
        return false;
    }

    // Map memory segment
    if ((shm->data = mmap(0, sizeof(shared_data_t), PROT_WRITE, MAP_SHARED, shm->fd, 0)) == (void *)-1)
    {
        return false;
    }
    return true;
}

//licensePlate initializing
void plateInit(carVector_t *carVector)
{
    carVector->size = 0;
    for (int i = 0; i < MAX_CAPACITY; i++)
    {
        strcpy(carVector->plateQueue[i], "empty");
    }
}

//adding the licensePlate into the file
void addPlate(carVector_t *carVector, char *plate)
{
    int old_size = carVector->size;
    strcpy(carVector->plateQueue[old_size], plate);
    carVector->size = old_size + 1;
}

//removing the last licensePlate from the file
void popPlate(carVector_t *carVector)
{
    int old_size = carVector->size;
    char old_data[MAX_CAPACITY][STORAGE_CAPACITY];
    for (int i = 0; i < old_size; i++)
    {
        strcpy(old_data[i], carVector->plateQueue[i]);
    }
    for (int i = 0; i < old_size - 1; i++)
    {
        strcpy(carVector->plateQueue[i], old_data[i + 1]);
    }
    carVector->size = old_size - 1;
}

//removing the random licensePlate from the file
void popRandom(carVector_t *carVector, int index)
{
    int old_size = carVector->size;
    char old_data[MAX_CAPACITY][STORAGE_CAPACITY];
    for (int i = 0; i < old_size; i++)
    {
        strcpy(old_data[i], carVector->plateQueue[i]);
    }
    for (int i = 0; i < index; i++)
    {
        strcpy(carVector->plateQueue[i], old_data[i]);
    }

    for (int i = index; i < old_size - 1; i++)
    {
        strcpy(carVector->plateQueue[i], old_data[i + 1]);
    }
    carVector->size = old_size - 1;
}

// Print plates
// void printPlate(carVector_t *carVector)
// {
//     printf("NUMBER OF PLATES ARE: %d\n", carVector->size);
//     for (int i = 0; i < carVector->size; i++)
//     {
//         printf("Plate number %d is: %s\n", i, carVector->plateQueue[i]);
//     }
// }

//generating the random number
double generateRandom(int lower, int upper)
{
    double num = (rand() % (upper - lower + 1)) + lower;
    return num;
}

//procedure of car entrance into the park
void *lpr_entrance(void *arg)
{
    int i = *(int *)arg;
    int j = 0;
    do 
    {
        pthread_mutex_lock(&shm.data->entrance[i].licensePlateReader.m);
        strcpy(shm.data->entrance[i].licensePlateReader.plate, startLPR);
        pthread_mutex_unlock(&shm.data->entrance[i].licensePlateReader.m);

        pthread_cond_signal(&shm.data->entrance[i].licensePlateReader.c);

        pthread_mutex_lock(&shm.data->entrance[i].licensePlateReader.m);
        while (!strcmp(shm.data->entrance[i].licensePlateReader.plate, startLPR))
        {
            pthread_cond_wait(&shm.data->entrance[i].licensePlateReader.c, &shm.data->entrance[i].licensePlateReader.m);
        }
        pthread_mutex_unlock(&shm.data->entrance[i].licensePlateReader.m);

        bool allowed = false;
        char toDisplay;

        for (int j = 0; j < 100; j++)
        {
            pthread_mutex_lock(&shm.data->entrance[i].licensePlateReader.m);
            int returnVal = strcmp(shm.data->entrance[i].licensePlateReader.plate, approved_plates[j]);
            pthread_mutex_unlock(&shm.data->entrance[i].licensePlateReader.m);
            if (returnVal != 0)
            {
                toDisplay = 'X';
                allowed = false;
            }
            else
            {
                allowed = true;
                break;
            }
        }

        if (allowed)
        {
            pthread_mutex_lock(&carMemoryM);
            int currentCapacity = carMemory.size;
            pthread_mutex_unlock(&carMemoryM);
            if (currentCapacity >= CAPACITY * LEVELS)
            {
                toDisplay = 'F';     
            }
            else
            {
                toDisplay = generateRandom(0, 4) + '0';              
            }
        }


        pthread_mutex_lock(&shm.data->entrance[i].informationSign.m);
        shm.data->entrance[i].informationSign.display = toDisplay;
        pthread_mutex_unlock(&shm.data->entrance[i].informationSign.m);


        if (toDisplay != 'F' && toDisplay != 'X')
        {

            pthread_mutex_lock(&shm.data->entrance[i].boomGate.m);
            entrDisplay[i] = shm.data->entrance[i].boomGate.status;
            pthread_mutex_unlock(&shm.data->entrance[i].boomGate.m);


            if (entrDisplay[i] != 'O')
            {
                pthread_mutex_lock(&shm.data->entrance[i].boomGate.m);
                shm.data->entrance[i].boomGate.status = 'R';
                pthread_mutex_unlock(&shm.data->entrance[i].boomGate.m);

                pthread_cond_signal(&shm.data->entrance[i].boomGate.c);
                pthread_mutex_lock(&shm.data->entrance[i].boomGate.m);
                while (shm.data->entrance[i].boomGate.status != 'O')
                {
                    pthread_cond_wait(&shm.data->entrance[i].boomGate.c, &shm.data->entrance[i].boomGate.m);
                }
                pthread_mutex_unlock(&shm.data->entrance[i].boomGate.m);
            }

            pthread_mutex_lock(&levelQueueMutex[i]);
            pthread_mutex_lock(&shm.data->entrance[i].licensePlateReader.m);
            addPlate(&levelQueue[i], shm.data->entrance[i].licensePlateReader.plate);
            pthread_mutex_unlock(&shm.data->entrance[i].licensePlateReader.m);
            pthread_mutex_unlock(&levelQueueMutex[i]);


            clock_t finish = (clock_t)generateRandom(100, 10000) * 1000;          
            printf("Information status sign is: %c\n", shm.data->entrance[i].informationSign.display);
            printf("%s permitted to park on level %c\n", shm.data->entrance[i].licensePlateReader.plate, shm.data->entrance[i].informationSign.display);
            printf("%s will park for %0.2fs\n", shm.data->entrance[i].licensePlateReader.plate, (double)finish / CLOCKS_PER_SEC);


            pthread_mutex_lock(&carMemoryM);
            pthread_mutex_lock(&shm.data->entrance[i].licensePlateReader.m);
            addCar(&carMemory, shm.data->entrance[i].licensePlateReader.plate, clock(), finish, i);
            pthread_mutex_unlock(&shm.data->entrance[i].licensePlateReader.m);
            pthread_mutex_unlock(&carMemoryM);
        }
        else
        {
            printf("Information status sign is: %c\n", shm.data->entrance[i].informationSign.display);
            printf("%s Rejected!\n", shm.data->entrance[i].licensePlateReader.plate);
        }
  
    }while(!fire);
    return 0;
}

//procedure of assigning the cars into the park according to the level
void *lpr_level(void *arg)
{
    int i = *(int *)arg;
    do
    {
        while (levelQueue[i].size <= 0)
            ;

        pthread_mutex_lock(&shm.data->level[i].licensePlateReader.m);
        strcpy(shm.data->level[i].licensePlateReader.plate, startLPR);
        pthread_mutex_unlock(&shm.data->level[i].licensePlateReader.m);


        pthread_cond_signal(&shm.data->level[i].licensePlateReader.c);

        pthread_mutex_lock(&shm.data->level[i].licensePlateReader.m);
        while (!strcmp(shm.data->level[i].licensePlateReader.plate, startLPR))
        {
            pthread_cond_wait(&shm.data->level[i].licensePlateReader.c, &shm.data->level[i].licensePlateReader.m);
        }
        pthread_mutex_unlock(&shm.data->level[i].licensePlateReader.m);
    
        pthread_mutex_lock(&carMemoryM);
        pthread_mutex_lock(&shm.data->level[i].licensePlateReader.m);
        char plate[7];
        strcpy(plate, shm.data->level[i].licensePlateReader.plate);
        while (findIndex(&carMemory, plate) == -1)
            ;
        int returnVal = carMemory.car[findIndex(&carMemory, plate)].count;
        pthread_mutex_unlock(&shm.data->level[i].licensePlateReader.m);
        pthread_mutex_unlock(&carMemoryM);


        if (returnVal == 0)
        {

            pthread_mutex_lock(&levelCapacityMutex[i]);
            int currentLevelCap = levelCapacity[i];
            pthread_mutex_unlock(&levelCapacityMutex[i]);

            if (currentLevelCap < CAPACITY)
            {

                usleep(10000);

 
                pthread_mutex_lock(&carMemoryM);
                pthread_mutex_lock(&shm.data->level[i].licensePlateReader.m);
                while (findIndex(&carMemory, plate) == -1)
                    ;
                carMemory.car[findIndex(&carMemory, shm.data->level[i].licensePlateReader.plate)].count = 1;
                pthread_mutex_unlock(&shm.data->level[i].licensePlateReader.m);
                pthread_mutex_unlock(&carMemoryM);
                pthread_mutex_lock(&levelCapacityMutex[i]);
                levelCapacity[i]++;
                pthread_mutex_unlock(&levelCapacityMutex[i]);
            }
            else
            {

                pthread_mutex_lock(&levelQueueMutex[i]);
                addPlate(&levelQueue[(int)generateRandom(0, 4)], plate);
                pthread_mutex_unlock(&levelQueueMutex[i]);
            }
        }
        else
        {

            pthread_mutex_lock(&exitQueueMutex[i]);
            pthread_mutex_lock(&shm.data->level[i].licensePlateReader.m);
            addPlate(&exitQueue[i], shm.data->level[i].licensePlateReader.plate);
            pthread_mutex_unlock(&shm.data->level[i].licensePlateReader.m);
            pthread_mutex_unlock(&exitQueueMutex[i]);
            // Decrease capacity
            pthread_mutex_lock(&levelCapacityMutex[i]);
            if (levelCapacity[i] != 0)
            {
                levelCapacity[i]--;
            }
            pthread_mutex_unlock(&levelCapacityMutex[i]);
        }
    }while (!fire);
    return 0;
}

//the controller for the level of the building
void *level_cont(void *arg)
{
    int i = *(int *)arg;
    do
    {

        pthread_mutex_lock(&shm.data->level[i].licensePlateReader.m);
        while (strcmp(shm.data->level[i].licensePlateReader.plate, startLPR))
        {
            pthread_cond_wait(&shm.data->level[i].licensePlateReader.c, &shm.data->level[i].licensePlateReader.m);
        }
        pthread_mutex_unlock(&shm.data->level[i].licensePlateReader.m);


        while (levelQueue[i].size <= 0)
            ;


        pthread_mutex_lock(&shm.data->level[i].licensePlateReader.m);
        pthread_mutex_lock(&levelQueueMutex[i]);
        strcpy(shm.data->level[i].licensePlateReader.plate, levelQueue[i].plateQueue[0]);
        popPlate(&levelQueue[i]);
        pthread_mutex_unlock(&shm.data->level[i].licensePlateReader.m);
        pthread_mutex_unlock(&levelQueueMutex[i]);


        pthread_cond_signal(&shm.data->level[i].licensePlateReader.c);
    }while (!fire);
    return 0;
}

//procedure of car exit from the park
void *lpr_exit(void *arg)
{
    int i = *(int *)arg;
    do
    {
        while (exitQueue[i].size <= 0)
            ;

        pthread_mutex_lock(&shm.data->exit[i].licensePlateReader.m);
        strcpy(shm.data->exit[i].licensePlateReader.plate, startLPR);
        pthread_mutex_unlock(&shm.data->exit[i].licensePlateReader.m);

        pthread_cond_signal(&shm.data->exit[i].licensePlateReader.c);

        pthread_mutex_lock(&shm.data->exit[i].licensePlateReader.m);
        while (!strcmp(shm.data->exit[i].licensePlateReader.plate, startLPR))
        {
            pthread_cond_wait(&shm.data->exit[i].licensePlateReader.c, &shm.data->exit[i].licensePlateReader.m);
        }
        pthread_mutex_unlock(&shm.data->exit[i].licensePlateReader.m);

        pthread_mutex_lock(&shm.data->exit[i].boomGate.m);
        exitDisplay[i] = shm.data->exit[i].boomGate.status;
        pthread_mutex_unlock(&shm.data->exit[i].boomGate.m);

        if (exitDisplay[i] != 'O')
        {
            pthread_mutex_lock(&shm.data->exit[i].boomGate.m);
            shm.data->exit[i].boomGate.status = 'R';
            pthread_mutex_unlock(&shm.data->exit[i].boomGate.m);

            pthread_cond_signal(&shm.data->exit[i].boomGate.c);
            pthread_mutex_lock(&shm.data->exit[i].boomGate.m);
            while (shm.data->exit[i].boomGate.status != 'O')
            {
                pthread_cond_wait(&shm.data->exit[i].boomGate.c, &shm.data->exit[i].boomGate.m);
            }
            pthread_mutex_unlock(&shm.data->exit[i].boomGate.m);
        }


        pthread_mutex_lock(&carMemoryM);
        pthread_mutex_lock(&shm.data->exit[i].licensePlateReader.m);   
        printf("%s leaves through exit %d.\n", shm.data->exit[i].licensePlateReader.plate, shm.data->exit[i + 1]);


        removeCar(&carMemory, shm.data->exit[i].licensePlateReader.plate);
        printf("%s has left the carpark.\n", shm.data->exit[i].licensePlateReader.plate);
        pthread_mutex_unlock(&shm.data->exit[i].licensePlateReader.m);
        pthread_mutex_unlock(&carMemoryM);
        generateBill(shm.data->exit[i].licensePlateReader.plate);
    }while (!fire);
    return 0;
}

//The controller for exit
void *exit_cont(void *arg)
{
    int i = *(int *)arg;
    do
    {

        pthread_mutex_lock(&shm.data->exit[i].licensePlateReader.m);
        while (strcmp(shm.data->exit[i].licensePlateReader.plate, startLPR))
        {
            pthread_cond_wait(&shm.data->exit[i].licensePlateReader.c, &shm.data->exit[i].licensePlateReader.m);
        }
        pthread_mutex_unlock(&shm.data->exit[i].licensePlateReader.m);

        while (exitQueue[i].size <= 0)
            ;


        pthread_mutex_lock(&shm.data->exit[i].licensePlateReader.m);
        pthread_mutex_lock(&exitQueueMutex[i]);
        strcpy(shm.data->exit[i].licensePlateReader.plate, exitQueue[i].plateQueue[0]);
        pthread_mutex_unlock(&shm.data->exit[i].licensePlateReader.m);


        popPlate(&exitQueue[i]);
        pthread_mutex_unlock(&exitQueueMutex[i]);


        usleep(10000);


        pthread_cond_signal(&shm.data->exit[i].licensePlateReader.c);
    }while (!fire);
    return 0;
}

// Check the time of a car is parked
void *time_check(void *arg)
{
    int i = *(int *)arg;
    do
    {
        while (carMemory.size <= 0)
            ;

        for (i = 0; i < carMemory.size; i++)
        {
            if (carMemory.car[i].withdrawn != true && (double)carMemory.car[i].start + carMemory.car[i].finish <= (double)clock())
            {
                carMemory.car[i].withdrawn = true;

                pthread_mutex_lock(&levelQueueMutex[carMemory.car[i].level]);
                addPlate(&levelQueue[carMemory.car[i].level], carMemory.car[i].plate);
                pthread_mutex_unlock(&levelQueueMutex[carMemory.car[i].level]);
            }
        }

        fire_checker();
    }while (!fire);

    fire_emergency();
    return 0;
}

// Entrance boom gate controller
void *boomgate_entrance(void *arg) 
{
    int i = *(int *)arg;
    // char *gatecontrol ="RO";
    do
    {

        pthread_mutex_lock(&shm.data->entrance[i].boomGate.m);
        while (shm.data->entrance[i].boomGate.status == 'C')
        {
            pthread_cond_wait(&shm.data->entrance[i].boomGate.c, &shm.data->entrance[i].boomGate.m);
        }
        pthread_mutex_unlock(&shm.data->entrance[i].boomGate.m);

        // for (char *p = gatecontrol; *p != '\0'; p++)
        // {
        //     pthread_mutex_lock(&shm.data->entrance[i].boomGate.m);
        //     shm.data->entrance[i].boomGate.status = *p;
        //     pthread_mutex_unlock(&shm.data->entrance[i].boomGate.m);
        //         if (shm.data->entrance[i].boomGate.status = 'R')
        //         {
        //             usleep(10000);
        //         }
        //         if (shm.data->entrance[i].boomGate.status = 'O')
        //         {
        //             pthread_cond_signal(&shm.data->entrance[i].boomGate.c);
        //             usleep(20000);
        //         }      
        // };
       

        pthread_mutex_lock(&shm.data->entrance[i].boomGate.m);
        shm.data->entrance[i].boomGate.status = 'R';
        pthread_mutex_unlock(&shm.data->entrance[i].boomGate.m);
        usleep(10000);


        pthread_mutex_lock(&shm.data->entrance[i].boomGate.m);
        shm.data->entrance[i].boomGate.status = 'O';
        pthread_mutex_unlock(&shm.data->entrance[i].boomGate.m);
        pthread_cond_signal(&shm.data->entrance[i].boomGate.c);


        usleep(20000);


        pthread_mutex_lock(&shm.data->entrance[i].boomGate.m);
        shm.data->entrance[i].boomGate.status = 'L';
        pthread_mutex_unlock(&shm.data->entrance[i].boomGate.m);
        usleep(10000);

 
        pthread_mutex_lock(&shm.data->entrance[i].boomGate.m);
        shm.data->entrance[i].boomGate.status = 'C';
        pthread_mutex_unlock(&shm.data->entrance[i].boomGate.m);
    }while (!fire);

    fire_emergency();
    return 0;
}

// Exit boom gate controller
void *boomgate_exit(void *arg)
{
    int i = *(int *)arg;
    do
    {

        pthread_mutex_lock(&shm.data->exit[i].boomGate.m);
        while (shm.data->exit[i].boomGate.status == 'C')
        {
            pthread_cond_wait(&shm.data->exit[i].boomGate.c, &shm.data->exit[i].boomGate.m);
        }
        pthread_mutex_unlock(&shm.data->exit[i].boomGate.m);

      
        pthread_mutex_lock(&shm.data->exit[i].boomGate.m);
        shm.data->exit[i].boomGate.status = 'R';
        pthread_mutex_unlock(&shm.data->exit[i].boomGate.m);
        usleep(10000);

      
        pthread_mutex_lock(&shm.data->exit[i].boomGate.m);
        shm.data->exit[i].boomGate.status = 'O';
        pthread_mutex_unlock(&shm.data->exit[i].boomGate.m);
        pthread_cond_signal(&shm.data->exit[i].boomGate.c);

      
        usleep(20000);

      
        pthread_mutex_lock(&shm.data->exit[i].boomGate.m);
        shm.data->exit[i].boomGate.status = 'L';
        pthread_mutex_unlock(&shm.data->exit[i].boomGate.m);
        usleep(10000);

      
        pthread_mutex_lock(&shm.data->exit[i].boomGate.m);
        shm.data->exit[i].boomGate.status = 'C';
        pthread_mutex_unlock(&shm.data->exit[i].boomGate.m);
    }while (!fire);

    fire_emergency();
    return 0;
}

// Generate bills based on the total time they are parked
void generateBill(char *numPlate)
{

    FILE *billingFile;
    clock_t startCar = carMemory.car[findIndex(&carMemory, numPlate)].start;
    bill = (double)(clock() - startCar) / CLOCKS_PER_SEC * 50; // FOR NOW
    moneyEarned += bill;
    printf("%s will be billed for $%.2f\n", numPlate, bill);


    if ((billingFile = fopen("bill.txt", "r")))
    {
        billingFile = fopen("bill.txt", "a");
        fprintf(billingFile, "%s  -  $%.2f\n", numPlate, bill);
        fclose(billingFile);
    }
    else
    {
        printf("create new billing.txt file...\n");
        billingFile = fopen("billing.txt", "w");
        fprintf(billingFile, "%s  -  $%.2f\n", numPlate, bill);
        fclose(billingFile);
    }
}

// Display the cuurent status of the parking lot on a frequently-updating screen
void *statDisplay(void *args)
{
    system("clear");
    char entrancePlateLPR[8];
    char levelPlateLPR[8];
    char entranceGateStatus;
    char entranceInfoSign;
    char exitPlateLPR[8];
    char exitGateStatus;

    for (;;)
    {

        for (int i = 0; i < ENTRANCES; i++)
        {
            pthread_mutex_lock(&shm.data->entrance[i].licensePlateReader.m);
            strcpy(entrancePlateLPR, shm.data->entrance[i].licensePlateReader.plate);
            pthread_mutex_unlock(&shm.data->entrance[i].licensePlateReader.m);

            pthread_mutex_lock(&shm.data->entrance[i].boomGate.m);
            entranceGateStatus = shm.data->entrance[i].boomGate.status;
            pthread_mutex_unlock(&shm.data->entrance[i].boomGate.m);

            pthread_mutex_lock(&shm.data->entrance[i].informationSign.m);
            entranceInfoSign = shm.data->entrance[i].informationSign.display;
            pthread_mutex_unlock(&shm.data->entrance[i].informationSign.m);


            printf("\nEntrance %d | ", i + 1);
            printf("LPR: %s | ", entrancePlateLPR);
            printf("Display: %c | ", entranceInfoSign);
            printf("BoomGate: %c\n", entranceGateStatus);
            
            
            // printf("----------------------------Entrance %d information----------------------------\n", i + 1);
            // printf("LPR plate: %s \n", entrancePlateLPR);
            // printf("Boom boomGate status: %c\n", entranceGateStatus);
            // printf("Information display: %c\n", entranceInfoSign);
        }

        for (int i = 0; i < EXITS; i++)
        {
            pthread_mutex_lock(&shm.data->exit[i].licensePlateReader.m);
            strcpy(exitPlateLPR, shm.data->exit[i].licensePlateReader.plate);
            pthread_mutex_unlock(&shm.data->exit[i].licensePlateReader.m);
            // Grab the exit LPR boomGate status
            pthread_mutex_lock(&shm.data->exit[i].boomGate.m);
            exitGateStatus = shm.data->exit[i].boomGate.status;
            pthread_mutex_unlock(&shm.data->exit[i].boomGate.m);

            // printf("------------------------------Exit %d information------------------------------\n", i + 1);
            // printf("LPR plate: %s\n", exitPlateLPR);
            // printf("Boom boomGate status: %c\n", exitGateStatus);

            printf("\nExit %d | ", i + 1);
            printf("LPR: %s | ", exitPlateLPR);
            printf("BoomGate: %c\n", exitGateStatus);
        }

        int j = 0;
        for (int i = 0; i < LEVELS; i++)
        {
            pthread_mutex_lock(&shm.data->level[i].licensePlateReader.m);
            strcpy(levelPlateLPR, shm.data->level[i].licensePlateReader.plate);
            pthread_mutex_unlock(&shm.data->level[i].licensePlateReader.m);
            rawData[i][j % ARSIZE] = shm.data->level[i].tempSensor;     

            // printf("-----------------------------Level %d information------------------------------\n", i + 1);
            // printf("Temperature Sensor status: %s\n", (shm.data->level[i].fireAlarm == '0') ? "No fire detected..." : "FIRE DETECTED!");
            // // Display Number of vehicles  and  maximum  capacity  on  each  level
            // printf("Currently %d cars on level %d, which has a maximum capacity of %d\n", levelCapacity[i], i + 1, CAPACITY);

            printf("\nLevel %d | ", i + 1);
            printf("Capacity: %d/%d cars | ",  levelCapacity[i], i + 1, CAPACITY);
            printf("LPR: %s\n", levelPlateLPR);
            printf("Alarm: %s\n", (shm.data->level[i].fireAlarm == '0') ? "No emergency" : "FIRE EMERGENCY!");
            printf("Temperature sensor: %d degrees\n", rawData[i][j % ARSIZE]);
     
        }
        printf("------------------------------------------------------------\n");
        printf("Total Revenue Billed at: $%.2f\n", moneyEarned); //, totalRevenue);

        usleep(50000);
        system("clear");
    }
}

// Initialize threads
void initThreads(void)
{

    pthread_mutexattr_t am;
    pthread_mutexattr_init(&am);
    pthread_mutexattr_setpshared(&am, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&carMemoryM, &am);
    storageInit(&carMemory);

    pthread_create(&statDisplayThread, NULL, &statDisplay, NULL);
    int i;

    for (i = 0; i < LEVELS; i++)
    {
        int *c = malloc(sizeof(int));
        *c = i;
        plateInit(&levelQueue[i]);
        pthread_mutex_init(&levelQueueMutex[i], NULL);
        pthread_mutex_init(&levelCapacityMutex[i], NULL);
        pthread_create(&timeCheckThread[i], NULL, &time_check, c);
        pthread_create(&lprThreadLevel[i], NULL, &lpr_level, c);
        pthread_create(&lprThreadLevelCont[i], NULL, &level_cont, c);
    }
   
    for (i = 0; i < EXITS; i++)
    {
        int *b = malloc(sizeof(int));
        *b = i;
        plateInit(&exitQueue[i]);
        pthread_mutex_init(&exitQueueMutex[i], NULL);
        pthread_create(&lprThreadExit[i], NULL, &lpr_exit, b);
        pthread_create(&lprThreadExitCont[i], NULL, &exit_cont, b);
        pthread_create(&boomgateThreadExit[i], NULL, &boomgate_exit, b);
    }

     for (i = 0; i < ENTRANCES; i++)
    {
        int *a = malloc(sizeof(int));
        *a = i;
        pthread_create(&lprThreadEntrance[i], NULL, &lpr_entrance, a);
        pthread_create(&boomgateThreadEntrance[i], NULL, &boomgate_entrance, a);
    }

    pthread_join(statDisplayThread, NULL);
     for (i = 0; i < LEVELS; i++)
    {
        pthread_join(timeCheckThread[i], NULL);
        pthread_join(lprThreadLevel[i], NULL);
        pthread_join(lprThreadLevelCont[i], NULL);
    }

    for (i = 0; i < ENTRANCES; i++)
    {
        pthread_join(lprThreadEntrance[i], NULL);
        pthread_join(boomgateThreadEntrance[i], NULL);
    }

    for (i = 0; i < EXITS; i++)
    {
        pthread_join(lprThreadExit[i], NULL);
        pthread_join(lprThreadExitCont[i], NULL);
        pthread_join(boomgateThreadExit[i], NULL);
    }

   
}

// Read licence plates file
void readFile(char *name)
{
    int i = 0;
    FILE *file = fopen(name, "r");
    while (fgets(approved_plates[i], 10, file))
    {
        approved_plates[i][strlen(approved_plates[i]) - 1] = '\0';
        i++;
    }
}

// Displays evacuation message
void evac_sequence(void)
{
    char *evacmessage = "EVACUATE";
    for (char *p = evacmessage; *p != '\0'; p++)
    {
        for (int i = 0; i < ENTRANCES; i++)
        {
            pthread_mutex_lock(&shm.data->entrance[i].informationSign.m);
            shm.data->entrance[i].informationSign.display = *p;
            pthread_mutex_unlock(&shm.data->entrance[i].informationSign.m);
            usleep(20000);
        }
    }
}

// Raise all boom gates and initialize evacuation sequence if a fire is reported 
void fire_emergency(void)
{
    char *raiseOpen = "RO";
    for (char *p = raiseOpen; *p != '\0'; p++)
    {
        for (int i = 0; i < ENTRANCES; i++)
        {
            pthread_mutex_lock(&shm.data->entrance[i].boomGate.m);
            shm.data->entrance[i].boomGate.status = *p;
            pthread_mutex_unlock(&shm.data->entrance[i].boomGate.m);
            usleep(10000);
        }

        for (int i = 0; i < EXITS; i++)
        {
            pthread_mutex_lock(&shm.data->exit[i].boomGate.m);
            shm.data->exit[i].boomGate.status = *p;
            pthread_mutex_unlock(&shm.data->exit[i].boomGate.m);
            usleep(10000);
        }
    }
    
    for (int i = 0; i < LEVELS; i++)
    {
        pthread_mutex_lock(&levelCapacityMutex[i]);
        levelCapacity[i] = 0;
        pthread_mutex_unlock(&levelCapacityMutex[i]);
    }
    while (fire)
    {
        evac_sequence();
    }
}

// Check if the fire alarm in each level is triggered to report a fire
void fire_checker(void)
{

    for (int i = 0; i < LEVELS; i++)
    {
        if (shm.data->level[i].fireAlarm == '1')
        {
            fire = 1;
        }
    }
}

// Main program
int main()
{
    // while(!fire)
    // {
        // Initialise random seed
        time_t t;
        srand((unsigned)time(&t));

        // Initialises all level queues
        for (int i = 0; i < LEVELS; i++)
        {
            levelCapacity[i] = 0;
        }
        moneyEarned = 0;
        fire = 0;

        // Read the number plates
        readFile("plates.txt");

        // Mount shared memory
        create_shared_object_R(&shm, SHARE_NAME);

        // Create and join threads
        initThreads();
    // }
    // fire_emergency();
}
