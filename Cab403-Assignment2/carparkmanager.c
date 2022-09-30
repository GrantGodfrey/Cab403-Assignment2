#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

#define LEVELS 5
#define ENTRANCES 5
#define EXITS 5

#define MEDIAN_WINDOW 5
#define TEMPCHANGE_WINDOW 30

struct boomgate
{
    pthread_mutex_t m;
    pthread_cond_t c;
    char s;
};

// bool check_lpr()

void *openboomgate(void *arg)
{
    struct boomgate *bg = arg;
    pthread_mutex_lock(&bg->m);
    for (;;)
    {
        if (bg->s == 'C')
        {
            bg->s = 'R';
            pthread_cond_broadcast(&bg->c);
        }
        if (bg->s == 'O')
        {
        }
        pthread_cond_wait(&bg->c, &bg->m);
    }
    pthread_mutex_unlock(&bg->m);
}