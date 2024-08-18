#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>


#define MAX_THREADS 10          // The maximum number of real threads that can run at the same time

sem_t buffer_mutex;             // Semaphore that controls the access to the BUFFER
sem_t password_mutex;           // Semaphore that controls the access to the password table
sem_t reader_count_mutex;       // Semaphore that controls the access to the reader count

int reader_count = 0;           // Number of readers that are currently reading
int BUFFER = 0;                 // Global variable. Readers read from it and writers write to it
int passwords[MAX_THREADS];     // Array for holding the passwords for the threads


typedef struct {
    int password;       // Holds the password for each thread
    int tid;            // Holds the thread no
    int is_reader;      // 1 for reader, 0 for writer
} thread_information;



void *reader_writer(void *args) {               // Function for reader-writers

    thread_information *thread_args = (thread_information *) args;

    int password = thread_args->password;
    int tid = thread_args->tid;                 // Assigning arguments that come from
    int is_reader = thread_args->is_reader;     // the creation of the thread
    

    char *job;
    if (is_reader) job = "reader";              // Assigning job of the
        else job = "writer";                    // thread as reader or writer

    int is_permitted = 0;                       // 1 for permitted, 0 for not permitted


    for (int i = 0; i < 5; i++) {               // Each reader-writer can do 5 operations
        sleep(1);                               // Sleep 1 second before consecutive writing-reading
                           
        sem_wait(&password_mutex);                  // While checking password for all the threads, we
        for (int k = 0; k < MAX_THREADS; k++) {     // lock access to passwords to check password safely.
            if (passwords[k] == password) {         // If it is a real thread, its password will match one
                is_permitted = 1;                   // of the passwords in the table and it will be permitted.
                break;
            }
        }
        sem_post(&password_mutex);                  // We release password access


        if (is_permitted) {                         // If it is a real thread, it is permitted

            if (is_reader) {                        // If it is a reader

                sem_wait(&reader_count_mutex);      // Manages reader count safely
                reader_count++;
                if (reader_count == 1) {            // This is the first reader
                    sem_wait(&buffer_mutex);        // that locks the buffer
                }
                sem_post(&reader_count_mutex);

                printf("%d          real                  %s               %d\n", tid, job, BUFFER);            // Print

                sem_wait(&reader_count_mutex);
                reader_count--;                     // Decrement reader count after it finishes reading
                if (reader_count == 0) {            // This is the last reader
                    sem_post(&buffer_mutex);        // that unlocks the buffer
                }
                sem_post(&reader_count_mutex);
            }
                else {                                                                                          // If it is a writer
                    sem_wait(&buffer_mutex);                                                                    // Lock BUFFER access for exclusive writing
                    BUFFER = rand() % 10000;                                                                    // Write a random number to the BUFFER
                    printf("%d          real                  %s               %d\n", tid, job, BUFFER);        // Print
                    sem_post(&buffer_mutex);                                                                    // Release BUFFER access
                }
        }
            else { printf("%d          dummy                 %s               No permission\n", tid, job); }    // If it is a dummy thread, don't do any operation
            
    }
    pthread_exit(NULL);     // Terminate the thread
}



int main() {
    srand(time(NULL));                                  // Seed random number generator
    pthread_t threads[2 * MAX_THREADS];                 // Array that stores the threads
    thread_information thread_args[2 * MAX_THREADS];    // Array that stores thread arguments

    sem_init(&buffer_mutex, 0, 1);          // Initialize the semaphore for BUFFER access
    sem_init(&password_mutex, 0, 1);        // Initialize the semaphore for password access
    sem_init(&reader_count_mutex, 0, 1);    // Initialize the semaphore for reader count

    
    for (int i = 0; i < MAX_THREADS; i++) {            // Fill the password table with
        passwords[i] = rand() % 900000 + 100000;       // 10 random 6 digit numbers
    }

    int num_readers;
    int num_writers;

    printf("Number of readers: ");          // Get the number of readers  
    scanf("%d", &num_readers);              // and writers from the user
    printf("Number of writers: ");
    scanf("%d", &num_writers);

    if(num_readers < 1 || num_writers < 1 || num_readers > 9 || num_writers > 9 || num_readers + num_writers > 10) {    
        printf("The number of reader/writer should be between 1-9.\n");                                                  
        printf("The total number of real readers and writers cannot exceed 10.");       // Checking the number of 
        return 1;                                                                       // readers and writers
    }

    printf("\nThread No  Validity(real/dummy)  job(reader/writer)  Value read/written\n");

    
    for (int i = 0; i < num_readers; i++) {                                             // Creating threads for real and dummy readers
        
        thread_args[i].password = passwords[i];                                         // Assigning unique password from the password table for real readers
        thread_args[i].tid = i + 1;                                                     // Assigning thread numbers for real readers from 1 to num_readers
        thread_args[i].is_reader = 1;                                                   // Indicating it is a reader
        pthread_create(&threads[i], NULL, reader_writer, (void *) &thread_args[i]);     // Creating real reader threads with the given arguments

        thread_args[i + MAX_THREADS].password = rand() % 900000 + 100000;                                           // Assigning unique password for dummy readers
        thread_args[i + MAX_THREADS].tid = i + 1;                                                                   // Assigning same thread numbers for dummy readers from 1 to num_readers
        thread_args[i + MAX_THREADS].is_reader = 1;                                                                 // Indicating it is a reader
        pthread_create(&threads[i + MAX_THREADS], NULL, reader_writer, (void *) &thread_args[i + MAX_THREADS]);     // Creating dummy reader threads with the given arguments
    }


    for (int i = 0; i < num_writers; i++) {                                                                         // Creating threads for real and dummy writers        

        thread_args[i + num_readers].password = passwords[i + num_readers];                                         // Assigning unique password from the password table for real writers
        thread_args[i + num_readers].tid = i + 1;                                                                   // Assigning thread numbers for real writers from 1 to num_writers
        thread_args[i + num_readers].is_reader = 0;                                                                 // Indicating it is a writer
        pthread_create(&threads[i + num_readers], NULL, reader_writer, (void *) &thread_args[i + num_readers]);     // Creating real writer threads with the given arguments

        thread_args[i + num_readers + MAX_THREADS].password = rand() % 900000 + 100000;                                                         // Assigning unique password for dummy writers
        thread_args[i + num_readers + MAX_THREADS].tid = i + 1;                                                                                 // Assigning same thread numbers for dummy writers from 1 to num_readers
        thread_args[i + num_readers + MAX_THREADS].is_reader = 0;                                                                               // Indicating it is a writer           
        pthread_create(&threads[i + num_readers + MAX_THREADS], NULL, reader_writer, (void *) &thread_args[i + num_readers + MAX_THREADS]);     // Creating dummy writer threads with the given arguments
    }



    for (int i = 0; i < num_readers + num_writers; i++) {
        pthread_join(threads[i], NULL);                         // Waiting for all real and
        pthread_join(threads[i + MAX_THREADS], NULL);           // dummy threads to complete
    }


    sem_destroy(&buffer_mutex);        // Destroying the semaphores
    sem_destroy(&password_mutex);      // after finishing everything
    sem_destroy(&reader_count_mutex);

    return 0;
}
