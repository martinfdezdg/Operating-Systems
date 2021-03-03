#include <stdio.h> 
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

#define NR_PHILOSOPHERS 50

pthread_t philosophers[NR_PHILOSOPHERS];
pthread_mutex_t forks[NR_PHILOSOPHERS];


void init()
{
    int i;
    for(i=0; i<NR_PHILOSOPHERS; i++)
        pthread_mutex_init(&forks[i],NULL);
    
}

void destroy()
{
    int i;
    for(i=0; i<NR_PHILOSOPHERS; i++)
        pthread_mutex_destroy(&forks[i]);
    
}

void think(int i) {
    printf("Philosopher %d thinking... \n" , i);
    sleep(random() % 10);
    printf("Philosopher %d stopped thinking!!! \n" , i);

}

void eat(int i) {
    printf("Philosopher %d eating... \n" , i);
    sleep(random() % 5);
    printf("Philosopher %d is not eating anymore!!! \n" , i);

}

void toSleep(int i) {
    printf("Philosopher %d sleeping... \n" , i);
    sleep(random() % 10);
    printf("Philosopher %d is awake!!! \n" , i);
    
}

void* philosopher(void* i)
{
    // id del filosofo
    int nPhilosopher = (int)i;
    // id del tenedor derecho (= id_filosofo)
    int right = nPhilosopher;
    // id del tenedor izquierdo (= (id_filosofo - 1)%(num_filosofos))
    int left = (nPhilosopher - 1 == -1) ? NR_PHILOSOPHERS - 1 : (nPhilosopher - 1);
    while(1)
    {
        think(nPhilosopher);
        
        // TRY TO GRAB BOTH FORKS (right and left)
        // Para solucionar el problema principal, los interbloqueos,
        // basta con conseguir que al menos un filosofo coja los tenedores
        // en orden inverso.
        if (nPhilosopher % 2 == 0){
            pthread_mutex_lock(&forks[right]);
            printf("Philosopher %d takes right fork!!! \n" , nPhilosopher);
            sleep(5); // fuerza posibles interbloqueos
            pthread_mutex_lock(&forks[left]);
            printf("Philosopher %d takes left fork!!! \n" , nPhilosopher);
        } else {
            pthread_mutex_lock(&forks[left]);
            printf("Philosopher %d takes left fork!!! \n" , nPhilosopher);
            sleep(5); // fuerza posibles interbloqueos
            pthread_mutex_lock(&forks[right]);
            printf("Philosopher %d takes right fork!!! \n" , nPhilosopher);
        }

        eat(nPhilosopher);
        
        // PUT FORKS BACK ON THE TABLE
        pthread_mutex_unlock(&forks[left]);
        printf("Philosopher %d leaves right fork!!! \n" , nPhilosopher);
        pthread_mutex_unlock(&forks[right]);
        printf("Philosopher %d leaves left fork!!! \n" , nPhilosopher);
        
        toSleep(nPhilosopher);
   }

}

int main()
{
    init();
    unsigned long i;
    for(i=0; i<NR_PHILOSOPHERS; i++)
        pthread_create(&philosophers[i], NULL, philosopher, (void*)i);
    
    for(i=0; i<NR_PHILOSOPHERS; i++)
        pthread_join(philosophers[i],NULL);
    destroy();
    return 0;
} 
