#include "sharedMemory.h"
#include <assert.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>

#define SHARE_NAME "PARKING"
#define FIRETOLERANCE 8
#define SIZE 30
#define LOOPLIMITER 1e9

int ALARM = 0;
int16_t rawData[LEVELS][SIZE] = {0};
int16_t smoothData[LEVELS][SIZE] = {0};
shared_memory_t shm;

bool readObject(shared_memory_t *shm, const char *share_name)
{
    shm->name = share_name;
    while ((shm->fd = shm_open(share_name, O_RDWR, 0)) < 0)
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

int fixed(int16_t arr[LEVELS][SIZE], int index)
{

    assert(ALARM == 0);
    int count_temp = 0;

    for (int i = 0; i < SIZE; i++)
    {
        if (arr[index][i] >= 58)
        {
            count_temp++;
            if (count_temp >= SIZE * 0.9)
            {
                ALARM = 1;
                return ALARM;
            }
        }
    }
    return 0;
}

int riseRate(int16_t arr[LEVELS][SIZE], int index)
{
    assert(ALARM == 0);
    if (arr[index][0] != 0 && arr[index][SIZE - 1] - arr[index][0] >= FIRETOLERANCE)
    {
        ALARM = 1;
        return ALARM;
    }
    return 0;
}

int16_t searchMedian(int16_t array[SIZE], int n)
{
    int16_t medianVal = 0;
    if (n % 2 == 0)
        medianVal = (array[(n - 1) / 2] + array[n / 2]) / 2.0;
    else
        medianVal = array[n / 2];

    return medianVal;
}

int16_t smoothedData(int16_t arr[5])
{
    int num = 5;
    arraySort(arr, num);
    int16_t median = searchMedian(arr, num);
    return median;
}

// https://www.includehelp.com/c-programs/calculate-median-of-an-array.aspx.
void arraySort(int16_t array[SIZE], int n)
{
    int16_t i = 0, j = 0, temp = 0;
    for (i = 0; i < n; i++)
    {
        for (j = 0; j < n - 1; j++)
        {
            if (array[j] > array[j + 1])
            {
                temp = array[j];
                array[j] = array[j + 1];
                array[j + 1] = temp;
            }
        }
    }
}

int main()
{
    readObject(&shm, SHARE_NAME);
    int j = 0, levelOfFire = 0, lim = 0;
    while (lim < LOOPLIMITER)
    {
        for (int i = 0; i < LEVELS; i++)
        {
            rawData[i][j % SIZE] = shm.data->level[i].tempSensor;
            printf("Temperature sensor for level %d: %d degrees\n", i + 1, rawData[i][j % SIZE]);
            if (j > 4)
            {
                int index = j % SIZE, z = 0, num = 5;
                int16_t mostRecentReadings[5];
                for (int currentIndex = index - num; currentIndex < index; currentIndex++)
                {
                    mostRecentReadings[z] = rawData[i][currentIndex];
                    z++;
                }
                int16_t medianReading = smoothedData(mostRecentReadings);
                smoothData[i][j % SIZE] = medianReading;
                if (fixed(smoothData, i) || riseRate(smoothData, i))
                {
                    levelOfFire = i;
                    break;
                }
            }
            usleep(2000);
            if (i >= LOOPLIMITER)
            {
                printf("Loop Limit reached!");
                exit(1);
            }
        }
        j++;
        if (ALARM == 1)
        {
            shm.data->level[levelOfFire].fireAlarm = '1';
            break;
        }
    }
}