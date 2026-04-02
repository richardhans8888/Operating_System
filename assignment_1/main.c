/*
 * COMP6697001 - Operating Systems | Assignment 1
 * Multi-threaded Producer-Consumer with Bounded Stack Buffer
 *
 * Compile : gcc -O2 -o main main.c -lpthread
 * Run     : time ./main
 *
 * Synchronization tools (Lecture 5):
 *   pthread_mutex_t  mutex      - mutual exclusion for all shared state
 *   pthread_cond_t   cond_even  - wakes even consumer when even is on top
 *   pthread_cond_t   cond_odd   - wakes odd  consumer when odd  is on top
 *   pthread_cond_t   cond_space - wakes producer when a stack slot is freed
 *
 * Condition variables are used instead of semaphores because two consumers
 * compete for the same stack top — each sleeps on its own named cond var
 * and only wakes when a matching parity item is available (Slide 17 pattern).
 */

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define LOWER_NUM 1
#define UPPER_NUM 10000
#define BUFFER_SIZE 100
#define MAX_COUNT 10000

static int buffer[BUFFER_SIZE];
static int top = -1;

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond_even = PTHREAD_COND_INITIALIZER;
static pthread_cond_t cond_odd = PTHREAD_COND_INITIALIZER;
static pthread_cond_t cond_space = PTHREAD_COND_INITIALIZER;

static int produced = 0, consumed = 0;
static FILE *f_all, *f_even, *f_odd;

static void signal_for_top(void) {
  if (top < 0)
    return;
  if (buffer[top] % 2 == 0)
    pthread_cond_signal(&cond_even);
  else
    pthread_cond_signal(&cond_odd);
}

static void *producer(void *arg) {
  (void)arg;
  for (int i = 0; i < MAX_COUNT; i++) {
    int num = (rand() % (UPPER_NUM - LOWER_NUM + 1)) + LOWER_NUM;
    fprintf(f_all, "%d\n", num);

    pthread_mutex_lock(&mutex);
    while (top >= BUFFER_SIZE - 1)
      pthread_cond_wait(&cond_space, &mutex);
    buffer[++top] = num;
    produced++;
    signal_for_top();
    pthread_mutex_unlock(&mutex);
  }

  pthread_mutex_lock(&mutex);
  pthread_cond_broadcast(&cond_even);
  pthread_cond_broadcast(&cond_odd);
  pthread_mutex_unlock(&mutex);
  return NULL;
}

/* parity 0 = even -> even.txt | parity 1 = odd -> odd.txt */
static void *consumer(void *arg) {
  int parity = *(int *)arg;
  FILE *fout = (parity == 0) ? f_even : f_odd;
  pthread_cond_t *my_cond = (parity == 0) ? &cond_even : &cond_odd;

  pthread_mutex_lock(&mutex);
  while (consumed < MAX_COUNT) {
    /* wait if buffer empty or top has wrong parity */
    while (consumed < MAX_COUNT && (top < 0 || buffer[top] % 2 != parity))
      pthread_cond_wait(my_cond, &mutex);

    if (consumed >= MAX_COUNT)
      break;

    int num = buffer[top--];
    consumed++;
    pthread_cond_signal(&cond_space);
    signal_for_top();

    /* each file has one owner — safe to write outside the lock */
    pthread_mutex_unlock(&mutex);
    fprintf(fout, "%d\n", num);
    pthread_mutex_lock(&mutex);
  }

  pthread_cond_broadcast(&cond_even);
  pthread_cond_broadcast(&cond_odd);
  pthread_mutex_unlock(&mutex);
  return NULL;
}

int main(void) {
  srand((unsigned int)time(NULL));

  f_all = fopen("all.txt", "w");
  f_even = fopen("even.txt", "w");
  f_odd = fopen("odd.txt", "w");
  if (!f_all || !f_even || !f_odd) {
    perror("fopen");
    return 1;
  }

  pthread_t prod_tid, even_tid, odd_tid;
  int parity_even = 0, parity_odd = 1;

  pthread_create(&prod_tid, NULL, producer, NULL);
  pthread_create(&even_tid, NULL, consumer, &parity_even);
  pthread_create(&odd_tid, NULL, consumer, &parity_odd);

  pthread_join(prod_tid, NULL);
  pthread_join(even_tid, NULL);
  pthread_join(odd_tid, NULL);

  fclose(f_all);
  fclose(f_even);
  fclose(f_odd);
  printf("Done. produced=%d  consumed=%d\n", produced, consumed);
  return 0;
}