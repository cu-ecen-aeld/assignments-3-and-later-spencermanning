#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include "time.h"

// Optional: use these functions to add debug or error prints to your application
#define DEBUG_LOG(msg,...) printf(msg, ##__VA_ARGS__)
//#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

void* threadfunc(void* thread_param)
{

    // TODO: wait, obtain mutex, wait, release mutex as described by thread_data structure
    // hint: use a cast like the one below to obtain thread arguments from your parameter
    //struct thread_data* thread_func_args = (struct thread_data *) thread_param;

    struct thread_data* thread_func_args = (struct thread_data *) thread_param;

    // False until made true
    thread_func_args->thread_complete_success = false;

    // Wait to obtain
    // sleep(thread_func_args->wait_to_obtain_ms);
    usleep((thread_func_args->wait_to_obtain_ms)*1000);
    DEBUG_LOG("---Waiting to obtain the mutex\n");

    // Obtain mutex
    if (pthread_mutex_lock(thread_func_args->mutex) != 0) {
        ERROR_LOG("---Mutex didn't get locked\n");
        DEBUG_LOG("---Mutex didn't get locked\n");
        return thread_func_args;
    }

    // Wait to release
    // sleep(thread_func_args->wait_to_release_ms);
    usleep((thread_func_args->wait_to_release_ms)*1000);
    DEBUG_LOG("---Waiting to release mutex\n");

    // Release mutex
    if (pthread_mutex_unlock(thread_func_args->mutex) != 0) {
        DEBUG_LOG("---Mutex didn't get unlocked\n");
        ERROR_LOG("---Mutex didn't get unlocked\n");
        return thread_func_args;
    }
    thread_func_args->thread_complete_success = true;
    
    return thread_func_args;
}


bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex,int wait_to_obtain_ms, int wait_to_release_ms)
{
    /**
     * TODO: allocate memory for thread_data, 
     * setup mutex and wait arguments, 
     * pass thread_data to created thread
     * using threadfunc() as entry point.
     *
     * return true if successful.
     *
     * See implementation details in threading.h file comment block
     */

    // Malloc the struct
    struct thread_data *thread_data_local;
    thread_data_local = malloc(sizeof(struct thread_data));

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 2400);

    thread_data_local->thread = thread;
    thread_data_local->mutex = mutex;
    thread_data_local->wait_to_obtain_ms = wait_to_obtain_ms;
    thread_data_local->wait_to_release_ms = wait_to_release_ms;

    int thread_status = pthread_create(thread, &attr, threadfunc, (void*)thread_data_local);

    if (thread_status) {
        DEBUG_LOG("---Thread created successfully!\n");
        return false;
    }
    else {
        DEBUG_LOG("---Thread not created!\n");
        return true;
    }
}

