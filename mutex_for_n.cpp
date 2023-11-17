#include <iostream>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

std::atomic<bool> done(false);

//sacar coisas da standard library
std::queue<int> buffer; //declara uma queue de ints chamada buffer
std::mutex mtx; //declara um mutex chamado mtx
std::condition_variable cv; //declara uma variavel de condição chamada cv. 
//Uma variável de condição é um objeto capaz de bloquear a thread até que uma condição seja satisfeita. 
//Ela é sempre usada em conjunto com um mutex, pois a condição é verificada enquanto o mutex está bloqueado.



void producer(int elem) {

   for (int i = 1; i <= elem; ++i) {
      int produced;
      {
         std::lock_guard<std::mutex> lock(mtx);
         buffer.push(i);
         produced = i;
         cv.notify_one();
      }
      std::cout << "Produced: " << produced << std::endl;         //tirei o cout fora do lock aui e no consumer
      std::this_thread::sleep_for(std::chrono::milliseconds(200));
   }
   done = true;
   cv.notify_all(); // notify all waiting threads that production is done
}

void consumer(int id, int num_items) {
   int i = 0;


   while (i < num_items) {

      //std::cout << "Consumer " << id << ": i = " << i << ", num_items = " << num_items << std::endl; // print i and num_items debug

      std::unique_lock<std::mutex> lock(mtx);

      cv.wait(lock, [] { return !buffer.empty() || done; }); // wait for notification from producer

      if (!buffer.empty() && !done) {
         int data = buffer.front();
         buffer.pop();
         lock.unlock(); // unlock before output to allow other consumers to proceed
         std::cout << "Consumer " << id << ": " << data << std::endl;

         //std::cout << "Buffer size: " << buffer.empty() << ", done = " << done << std::endl; // print buffer size and done debug

         //std::this_thread::sleep_for(std::chrono::milliseconds(1000));
      }
      ++i;
   }

}
int main() {
    int n;
    std::cout << "How many consumers do you want? ";
    std::cin >> n; 

   int elem;
   std::cout << "How many elements do you want to produce? ";
   std::cin >> elem;

   int divid = 1 + (elem/n); //dividir o numero de elementos pelo numero de consumidores

    std::thread producerThread(producer, elem); 

    std::thread consumerThreads[n]; //criar um array com n threads de consumidor

    for(int i = 0; i < n; i++){
        consumerThreads[i] = std::thread(consumer, i, divid); // pass i as the id argument and n as the num_items argument
    }
    
    producerThread.join(); 

    for (int i = 0; i < n; i++){
        consumerThreads[i].join();
    } 

    return 0;
}

//nao sei se devia usar um dynamic array para os consumidores
