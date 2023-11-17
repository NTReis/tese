#include <iostream>
#include <queue>
#include <thread>
#include <semaphore.h>
#include <atomic>



std::queue<int> buffer; //uma queue para guardar ints
sem_t emptySlots; //um semaforo para guardar o numero de slots vazios
sem_t fullSlots; //um semaforo para guardar o numero de slots cheios

void producer(int num_items) { //criar o produtor
   for (int i = 1; i <= num_items; ++i) {
      sem_wait(&emptySlots); //espera até que haja um slot vazio se nao houver fica à espera
      buffer.push(i); //adiciona o int i à queue
      std::cout << "Produced: " << i << std::endl;
      sem_post(&fullSlots); //incrementa o numero de slots cheios
      std::this_thread::sleep_for(std::chrono::milliseconds(200));
   }
   
}

void consumer(int id, int num_items) {
   int i = 0; // initialize a counter

   while (i < num_items) { // consume only num_items items

      //std::cout << "Consumer " << id << ": i = " << i << ", num_items = " << num_items << std::endl; // print i and num_items debug

      sem_wait(&fullSlots); // wait until there is a full slot
      int data = buffer.front(); // store the data of the first element
      buffer.pop();  // remove the first element from the queue
      std::cout << "Consumer " << id << ": " << data << std::endl; 
      sem_post(&emptySlots); // increment the number of empty slots
         //std::this_thread::sleep_for(std::chrono::milliseconds(1000));    
     
      ++i; // increment the counter
   }
}

int main() {
   int num_consumers, num_tasks;

   std::cout << "How many consumers?: ";
   std::cin >> num_consumers; 

   std::cout << "How many tasks to produce/consume?: ";
   std::cin >> num_tasks; 

   int divid = num_tasks/num_consumers; //dividir o numero de elementos pelo numero de consumidores

   sem_init(&emptySlots, 0, num_tasks);  // Maximum num_tasks empty slots in the buffer
   sem_init(&fullSlots, 0, 0);   // Initially, no full slots in the buffer

   std::thread producerThread(producer, num_tasks); // start producer thread
   
   std::thread consumerThreads[num_consumers]; // create an array of consumer threads


   for (int i = 0; i < num_consumers-1; ++i) {
      consumerThreads[i] = std::thread(consumer, i, divid); // start consumer threads
   }
   if( (num_tasks % num_consumers) == 0){
         consumerThreads[num_consumers-1] = std::thread(consumer, num_consumers-1, divid); // start consumer threads
      } else {
         consumerThreads[num_consumers-1] = std::thread(consumer, num_consumers-1, divid + (num_tasks%num_consumers) ); // start consumer threads
      }



   for (int i = 0; i < num_consumers; ++i) {
      consumerThreads[i].join(); // wait for each consumer thread to finish
   }

   producerThread.join(); // wait for producer thread to finish

   sem_destroy(&emptySlots);
   sem_destroy(&fullSlots);

   return 0;
}


//acho que o done acabou por não fazer nada
//consegui dividir as tasks direito e acho que isso resolveu o problema do consumer estar preso
