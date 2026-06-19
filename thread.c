// File thread.c, gestisce il pool di thread

#include <stdlib.h>

#include "data.h"
#include "measurement.h"
#include "thread.h"

static void *tpool_worker(void *arg);
static ThreadPoolWork *tpool_work_create(thread_func_t func, void *arg);
static void tpool_work_destroy(ThreadPoolWork *twork);
static ThreadPoolWork *tpool_work_get(ThreadPool *tpool);

/**
 * Crea un nuovo task per il ThreadPool
 * @param func Funzione da eseguire
 * @param arg Argomenti della funzione
 * @return Il ThreadPoolWork appena creato in caso di successo e NULL altrimenti
 */
static ThreadPoolWork *tpool_work_create(thread_func_t func, void *arg) {
  if (func == NULL)
    return NULL;

  ThreadPoolWork *twork = malloc(sizeof(*twork));
  twork->func = func;
  twork->arg = arg;
  twork->next = NULL;
  return twork;
}

/**
 * Distrugge il ThreadPoolWork
 * @param twork Il ThreadPoolWork da liberare
 */
static void tpool_work_destroy(ThreadPoolWork *twork) {
  if (twork == NULL)
    return;
  free(twork);
}

/**
 * Ottiene un ThreadPoolWork dalla coda del ThreadPool
 * @param tpool Il ThreadPool da cui ottenere il ThreadPoolWork
 * @return Il ThreadPoolWork modificato in caso di successo e NULL altrimenti
 */
static ThreadPoolWork *tpool_work_get(ThreadPool *tpool) {

  if (tpool == NULL)
    return NULL;

  ThreadPoolWork *work = tpool->work_first;

  if (work == NULL)
    return NULL;

  if (work->next == NULL) {
    tpool->work_first = NULL;
    tpool->work_last = NULL;
  } else {
    tpool->work_first = work->next;
  }

  return work;
}

/**
 * Funzione eseguita dai ThreadPoolWork del ThreadPool: preleva i task dalla
 * coda e li esegue finché il ThreadPool non viene fermato.
 * @param arg Il puntatore al ThreadPool
 */
static void *tpool_worker(void *arg) {
  ThreadPool *tpool = arg;

  while (1) {
    pthread_mutex_lock(&(tpool->work_mutex));

    // Attesa di nuovi task o segnale di stop
    while (tpool->work_first == NULL && !tpool->stop)
      pthread_cond_wait(&(tpool->work_cond), &(tpool->work_mutex));

    if (tpool->stop)
      break;

    // Estrazione del task dalla coda
    ThreadPoolWork *work = tpool_work_get(tpool);
    tpool->working_cnt++;
    pthread_mutex_unlock(&(tpool->work_mutex));

    if (work != NULL) {
      work->func(work->arg); // Esecuzione del task
      tpool_work_destroy(work);
    }

    pthread_mutex_lock(&(tpool->work_mutex));
    tpool->working_cnt--;

    // Segnala se tutti i task sono stati completati
    if (!tpool->stop && tpool->working_cnt == 0 && tpool->work_first == NULL)
      pthread_cond_signal(&(tpool->working_cond));
    pthread_mutex_unlock(&(tpool->work_mutex));
  }

  tpool->num_threads--;
  pthread_cond_signal(&(tpool->working_cond));
  pthread_mutex_unlock(&(tpool->work_mutex));
  return NULL;
}

/**
 * Crea e inizializza un nuovo thread pool.
 * @param num_threads Numero di thread nel ThreadPool
 * @return Puntatore al thread pool creato.
 */
ThreadPool *tpool_create(size_t num_threads) {
  pthread_t thread;

  if (num_threads == 0)
    num_threads = 2;

  ThreadPool *tpool = calloc(1, sizeof(*tpool));
  tpool->num_threads = num_threads;

  pthread_mutex_init(&(tpool->work_mutex), NULL);
  pthread_cond_init(&(tpool->work_cond), NULL);
  pthread_cond_init(&(tpool->working_cond), NULL);

  tpool->work_first = NULL;
  tpool->work_last = NULL;

  // Creazione dei thread in stato detached
  for (size_t i = 0; i < num_threads; i++) {
    pthread_create(&thread, NULL, tpool_worker, tpool);
    pthread_detach(thread);
  }

  return tpool;
}

/**
 * Ferma e distrugge il ThreadPool, liberando le risorse.
 * @param tpool Puntatore al ThreadPool da distruggere.
 */
void tpool_destroy(ThreadPool *tpool) {

  if (tpool == NULL)
    return;

  pthread_mutex_lock(&(tpool->work_mutex));
  ThreadPoolWork *work = tpool->work_first;
  ThreadPoolWork *work2 = NULL;
  while (work != NULL) {
    work2 = work->next;
    tpool_work_destroy(work);
    work = work2;
  }
  tpool->work_first = NULL;
  tpool->stop = true;
  pthread_cond_broadcast(&(tpool->work_cond));
  pthread_mutex_unlock(&(tpool->work_mutex));

  tpool_wait(tpool); // Attesa fine esecuzione thread

  pthread_mutex_destroy(&(tpool->work_mutex));
  pthread_cond_destroy(&(tpool->work_cond));
  pthread_cond_destroy(&(tpool->working_cond));

  free(tpool);
}

/**
 * Aggiunge un nuovo task alla coda del ThreadPool.
 * @param tpool Puntatore al ThreadPool.
 * @param func Funzione da eseguire.
 * @param arg Argomento della funzione.
 * @return Se il task è stato aggiunto, "true", altrimenti "false".
 */
bool tpool_add_work(ThreadPool *tpool, thread_func_t func, void *arg) {

  if (tpool == NULL)
    return false;

  ThreadPoolWork *work = tpool_work_create(func, arg);
  if (work == NULL)
    return false;

  pthread_mutex_lock(&(tpool->work_mutex));
  if (tpool->work_first == NULL) {
    tpool->work_first = work;
    tpool->work_last = tpool->work_first;
  } else {
    tpool->work_last->next = work;
    tpool->work_last = work;
  }

  pthread_cond_broadcast(&(tpool->work_cond));
  pthread_mutex_unlock(&(tpool->work_mutex));

  return true;
}

/**
 * Attende il completamento di tutti i task attualmente in coda.
 * @param tpool Puntatore al ThreadPool.
 */
void tpool_wait(ThreadPool *tpool) {
  if (tpool == NULL)
    return;

  pthread_mutex_lock(&(tpool->work_mutex));
  while (1) {
    if (tpool->work_first != NULL ||
        (!tpool->stop && tpool->working_cnt != 0) ||
        (tpool->stop && tpool->num_threads != 0)) {
      pthread_cond_wait(&(tpool->working_cond), &(tpool->work_mutex));
    } else {
      break;
    }
  }
  pthread_mutex_unlock(&(tpool->work_mutex));
}

/**
 * Funzione principale per l'esecuzione multithreaded del simulatore.
 * @param circ Circuito quantistico.
 * @param v_out Vettore di output.
 * @param num_threads Numero di thread.
 * @return 0 in caso di successo, altrimenti codice di errore.
 */
int multithread(QuantumCircuit circ, Complex *v_out, size_t num_threads) {
  ThreadPool *tpool = tpool_create(num_threads);

  // Evoluzione Unitaria
  if (calculate_circuit(circ, v_out, num_threads, tpool) != 0) {
    tpool_destroy(tpool);
    return 1;
  }

  if (circ.repetitions == 0) {
    print_vector(v_out, DIM(circ.qubits));
  }

  // Misurazione
  if (measure_circuit(v_out, num_threads, circ.repetitions, DIM(circ.qubits),
                      tpool) != 0) {
    tpool_destroy(tpool);
    return 2;
  }

  tpool_destroy(tpool);
  return 0;
}