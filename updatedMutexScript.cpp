#include <iostream>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <unistd.h>
#include <random>
#include <atomic>
#include <fstream>
#include "Task.h"
#include "Consumer.h"

std::mutex mtx;
std::mutex idMutex;    // Mutex to protect the increment operation for globalTaskId
std::mutex consumerMutex;
std::mutex workloadMutex;
std::deque<Task> taskBuffer;
std::vector<std::deque<Task>> taskBufferConsumer;
std::vector<int>* workload; // Vector to store the workload of each consumer
std::condition_variable cv;
std::condition_variable producer_cv;  // new condition variable for the scheduler
std::atomic<bool> producersFinished = false;
int globalTaskId = 0;  // Global variable to store the task ID



void producer(int id, int elem) {

    for (int i = 0; i < elem; ++i) {
        
            std::unique_lock<std::mutex> lock(idMutex);
            int taskId = globalTaskId++;  // Get the current ID and increment it

            lock.unlock();

            std::unique_lock<std::mutex> taskLock(mtx);
            taskBuffer.push_back(Task(taskId));
            taskLock.unlock();
            
            std::cout << "Producer" << id << ": " << taskId << std::endl;
                    
    }


}


void scheduler(int consumers) {
    
    while (!producersFinished) {
        std::this_thread::sleep_for(std::chrono::microseconds(10)); // Sleep to prevent race conditions
    }

    std::unique_lock<std::mutex> lock(mtx);

    while(!taskBuffer.empty()){

        producer_cv.wait(lock, []{ return !taskBuffer.empty(); }); 


        std::cout << "LOCK" << std::endl;
            
        int totalTasks = taskBuffer.size();
        int consumer_workload = totalTasks / consumers;
        int consumer_remainder = totalTasks % consumers;

        lock.unlock();
        for (int i = 0; i < consumers; ++i) {
            int sharedtask = consumer_workload + (i < consumer_remainder ? 1 : 0);

            std::cout << "Scheduler distributing " << sharedtask << " tasks to Consumer" << i << std::endl;

            for (int j = 0; j < sharedtask && !taskBuffer.empty(); ++j) {
                    
                std::unique_lock<std::mutex> lock(consumerMutex);
                producer_cv.wait(lock, [&] { return !taskBuffer.empty(); });

                Task task = taskBuffer.front();
                taskBuffer.pop_front();

                taskBufferConsumer[i].push_back(task);

                lock.unlock();

                std::unique_lock<std::mutex> workloadlock(workloadMutex);

                (*workload)[i] = taskBufferConsumer[i].size();

                workloadlock.unlock();
                
            }

        } 

        cv.notify_all();

    }
}



void consumer(int id, Consumer consumer) {
    std::unique_lock<std::mutex> workloadlock(workloadMutex);

    cv.wait(workloadlock, [&] { return (*workload)[id] > 0; });

    while ((*workload)[id] > 0) {
        std::unique_lock<std::mutex> lock(consumerMutex);

        cv.wait(lock, [&] { return !taskBufferConsumer[id].empty(); });

        if (!taskBufferConsumer[id].empty() && (*workload)[id] > 0) {
            Task task = taskBufferConsumer[id].front();
            taskBufferConsumer[id].pop_front();
            std::cout << "Consumer" << id << ": " ;

            double clk_frq = consumer.clock_freqs[id].front();

            task.run(clk_frq);


            --(*workload)[id];

            
        }

        lock.unlock();
    }
}


int main(int argc, char* argv[]) {
    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " <num_consumers> <num_producers> <num_elements>\n";
        return 1;
    }

    int consumers= std::stoi(argv[1]);
    int producers= std::stoi(argv[2]);
    int elem = std::stoi(argv[3]);



    //We have to have a static number of consumers when using this method
    //Load consumers from file 
    Consumer cons(consumers);
    cons.loadFreqs();




    int consumer_workload = (elem / consumers); 
    int producer_workload = (elem / producers);
    int producer_remainder = elem % producers;

    

    if (consumer_workload == 0) {
        std::cout << "WARNING: The number of consumers is greater than the number of elements to be produced. The number of consumers will be reduced from " << consumers << " to " << elem << ".\n" << std::endl;
        consumers = elem;
    } 
    if (producer_workload == 0) {
        std::cout << "WARNING: The number of producers is greater than the number of elements to be produced. The number of producers will be reduced from " << producers << " to " << elem << ".\n" << std::endl;
        producers = elem;
    }

    taskBufferConsumer.resize(consumers); // Initialize the task buffer for each consumer

    std::thread producerThreads[producers];

    std::thread consumerThreads[consumers];

    
    workload = new std::vector<int>(consumers, 0); // Initialize the workload vector


    for (int i = 0; i < producers; ++i) {
        int workload = producer_workload + (i < producer_remainder ? 1 : 0);
        producerThreads[i] = std::thread(producer, i, workload);
    }


    std::thread schedulerThread(scheduler, consumers);
        

    for (int i = 0; i < consumers; ++i) {
        consumerThreads[i] = std::thread(consumer, i, cons);
    }

 

    for (int i = 0; i < producers; i++) {
        producerThreads[i].join();
    }

    producersFinished = true;

    //producer acabava depois do scheduler começar então meti aqui a função notify_one em vez de estar no producer
    std::cout << "UNLOCK" << std::endl;
    producer_cv.notify_one();
    
 
    schedulerThread.join();

    for (int i = 0; i < consumers; i++) {
        consumerThreads[i].join();
    }

    delete workload; // Free the memory allocated for the workload vector


    return 0;
}
