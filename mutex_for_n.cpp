#include <iostream>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <unistd.h>
#include <random>




class Tasks {       
  public:            
    int duration;   

    bool regular; 

        // Constructor
    Tasks() {
        // Automatically set 'regular' to true or false randomly
        setRandomRegular();
    }


    void run() {
        if (regular) {
            //std::cout << "Regular task \n\n";
            usleep(200);
        } else {
            //std::cout << "Irregular task: ";
            std::random_device rd;
            std::mt19937 gen(rd());
            std::normal_distribution<double> distribution(0.51, 0.5);

            double random_value = distribution(gen);
            usleep(static_cast<int>(random_value * 200));

            //std::cout << random_value << "\n" << std::endl;
        }
    }

  private:
  void setRandomRegular() {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<int> distribution(0, 1);

        regular = distribution(gen);  // 0 corresponds to false, 1 corresponds to true
    }
};


std::queue<int> buffer; //declara uma queue de ints chamada buffer
std::mutex mtx; //declara um mutex chamado mtx
std::condition_variable cv; //declara uma variavel de condição chamada cv. 

void producer(int elem) {
 
   for (int i = 1; i <= elem; ++i) {
      int produced;
      {
         std::lock_guard<std::mutex> lock(mtx);
         buffer.push(i);
         produced = i;
         cv.notify_one();
      }
      std::cout << "Produced: " << produced << std::endl;         //tirei o cout fora do lock aqui e no consumer
      std::this_thread::sleep_for(std::chrono::milliseconds(200));
   }

   cv.notify_all(); // notify all waiting threads that production is done
}

void consumer(int id, int num_items) {
   int i = 0;


   while (i < num_items) {


      std::unique_lock<std::mutex> lock(mtx);

      cv.wait(lock, [] { return !buffer.empty(); }); // wait for notification from producer

      if (!buffer.empty()) {
         int data = buffer.front();
         buffer.pop();
         lock.unlock(); // unlock before output to allow other consumers to proceed
         std::cout << "Consumer " << id << ": " << data << std::endl;


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

   int divid = (elem/n); //dividir o numero de elementos pelo numero de consumidores

   std::thread producerThread(producer, elem);
   
   std::thread consumerThreads[n]; //criar um array com n threads de consumidor

   for (int i = 0; i < n-1; ++i) {
      consumerThreads[i] = std::thread(consumer, i, divid); // start consumer threads
   }
   if( (elem % n) == 0){
         consumerThreads[n-1] = std::thread(consumer, n-1, divid); // start consumer threads
      } else {
         consumerThreads[n-1] = std::thread(consumer, n-1, divid + (elem%n)); // start consumer threads
      }
    
   producerThread.join(); //assim tanto o produtor como os consumidores trabalham ao mesmo tempo se puser em cima da erro e só produz

   for (int i = 0; i < n; i++){
      consumerThreads[i].join();
   } 

   return 0;
}

