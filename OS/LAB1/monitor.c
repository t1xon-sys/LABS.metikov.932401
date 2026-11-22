#define _DEFAULT_SOURCE
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct payload
{
    int values[3];
};

struct event
{
    int id;
    char *text;               
    struct payload *payload;  
};

static struct event *event_create(int id, const char *text)
{
    struct event *e = malloc(sizeof *e);
    if (e == NULL)
        return NULL;
    e->id = id;
    e->text = strdup(text);
    e->payload = malloc(sizeof *e->payload);
    if (e->text == NULL || e->payload == NULL)
    {
        free(e->text);
        free(e->payload);
        free(e);
        return NULL;
    }
    e->payload->values[0] = id;
    e->payload->values[1] = id * id;
    e->payload->values[2] = id * id * id;
    return e;
}

static void event_destroy(struct event *e)
{
    if (e == NULL)
        return;
    free(e->text);
    free(e->payload);
    free(e);
}

struct monitor
{
    pthread_mutex_t mutex;
    pthread_cond_t condition;
    bool ready;          
    bool stopping;                         
    struct event *data;  
};

static void monitor_init(struct monitor *m)
{
    pthread_mutex_init(&m->mutex, NULL);
    pthread_cond_init(&m->condition, NULL);
    m->ready = false;
    m->stopping = false;
    m->data = NULL;
}

static void monitor_destroy(struct monitor *m)
{
    
    event_destroy(m->data);
    pthread_cond_destroy(&m->condition);
    pthread_mutex_destroy(&m->mutex);
}


static bool monitor_provide(struct monitor *m, struct event *e)
{
    pthread_mutex_lock(&m->mutex);
    if (m->ready)
    {
        pthread_mutex_unlock(&m->mutex);
        return false;
    }
    m->data = e;
    m->ready = true;
    printf("Поставщик: отправлено событие #%d\n", e->id);
    fflush(stdout);
    pthread_cond_signal(&m->condition);
    pthread_mutex_unlock(&m->mutex);
    return true;
}


static bool monitor_consume(struct monitor *m)
{
    struct event *e;

    pthread_mutex_lock(&m->mutex);
    while (!m->ready && !m->stopping)                
        pthread_cond_wait(&m->condition, &m->mutex); 

    if (!m->ready) 
    {
        pthread_mutex_unlock(&m->mutex);
        return false;
    }

    e = m->data; 
    m->data = NULL;
    m->ready = false;

    printf("Потребитель: получено событие #%d (\"%s\", данные: %d %d %d)\n",
           e->id, e->text,
           e->payload->values[0], e->payload->values[1], e->payload->values[2]);
    fflush(stdout);

    pthread_mutex_unlock(&m->mutex);

    event_destroy(e); 
    return true;
}


static void monitor_stop(struct monitor *m)
{
    pthread_mutex_lock(&m->mutex);
    m->stopping = true;
    pthread_cond_broadcast(&m->condition);
    pthread_mutex_unlock(&m->mutex);
}

struct context
{
    struct monitor *monitor;
    int event_count;
};

static void *producer_thread(void *arg)
{
    struct context *ctx = arg;
    for (int i = 1; i <= ctx->event_count; ++i)
    {
        struct event *e;

        sleep(1); 
        e = event_create(i, "событие от поставщика");
        if (e == NULL)
        {
            perror("event_create");
            break;
        }
        while (!monitor_provide(ctx->monitor, e)) 
            usleep(10000);                        
    }
    monitor_stop(ctx->monitor);
    return NULL;
}

static void *consumer_thread(void *arg)
{
    struct context *ctx = arg;
    while (monitor_consume(ctx->monitor))
        ;
    return NULL;
}

int main(int argc, char *argv[])
{
    struct monitor monitor;
    struct context ctx;
    pthread_t producer, consumer;
    int count = 10; 

    if (argc > 1)
        count = atoi(argv[1]);

    monitor_init(&monitor);
    ctx.monitor = &monitor;
    ctx.event_count = count;

    if (pthread_create(&consumer, NULL, consumer_thread, &ctx) != 0)
    {
        perror("pthread_create");
        return 1;
    }
    if (pthread_create(&producer, NULL, producer_thread, &ctx) != 0)
    {
        perror("pthread_create");
        monitor_stop(&monitor);
        pthread_join(consumer, NULL);
        monitor_destroy(&monitor);
        return 1;
    }

    pthread_join(producer, NULL);
    pthread_join(consumer, NULL);
    monitor_destroy(&monitor);

    printf("Работа завершена.\n");
    return 0;
}