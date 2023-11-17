#include <iostream>
#include <queue>
#include <thread>
#include <semaphore.h>

std::queue<int> buffer; //uma queue para guardar ints
sem_t emptySlots; //um semaforo para guardar o numero de slots vazios
sem_t fullSlots; //um semaforo para guardar o numero de slots cheios

void producer() { //criar o produtor
   for (int i = 1; i <= 5; ++i) {
      sem_wait(&emptySlots); //espera até que haja um slot vazio se nao houver fica à espera
      buffer.push(i); //adiciona o int i à queue
      std::cout << "Produced: " << i << std::endl;
      sem_post(&fullSlots); //incrementa o numero de slots cheios
      std::this_thread::sleep_for(std::chrono::milliseconds(500));
   }
}

void consumer() { //criar o consumidor
   while (true) {
      sem_wait(&fullSlots); //espera até que haja um slot cheio se nao houver fica à espera
      int data = buffer.front(); //dá store na data do primeiro elemento
      buffer.pop();  //remove o primeiro elemento da queue com pop
      std::cout << "Consumed: " << data << std::endl; 
      sem_post(&emptySlots); //incrementa o numero de slots vazios
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
   }
}

int main() {
   
   
   
   sem_init(&emptySlots, 0, 5);  // Maximum 5 empty slots in the buffer
   sem_init(&fullSlots, 0, 0);   // Initially, no full slots in the buffer

   std::thread producerThread(producer);
   std::thread consumerThread(consumer);

   producerThread.join();
   consumerThread.join();

   sem_destroy(&emptySlots);
   sem_destroy(&fullSlots);

   return 0;
}
