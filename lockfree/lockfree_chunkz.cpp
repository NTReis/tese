// g++ LockFree.cpp -o engine -lpthread -I/home/hondacivic/Boost/boost_1_82_0

#include <iostream>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <unistd.h>
#include <random>
#include <atomic>
#include "boost/lockfree/queue.hpp"
#include <fstream>
#include "Task.h"
#include "Consumer.h"
#include "Scheduler.h"
#include <boost/lockfree/spsc_queue.hpp>
#include <chrono>


std::mutex idMutex; // Mutex to protect the increment operation for globalTaskId
std::mutex printMutex; // Mutex to protect the print operation
boost::lockfree::queue<Task*, boost::lockfree::fixed_sized<false>> taskBuffer(0);
std::vector<Consumer> consumerlist;
//std::condition_variable cv;
//std::condition_variable producer_cv;  // new condition variable for the scheduler
std::atomic<bool> producersFinished=false;
std::atomic<bool> schedulersFinished=false;
std::atomic<bool> schedulersEnded=false;
std::atomic<bool> useProducer=false;
int globalTaskId = 0;  // Global variable to store the task ID
std::atomic<int> taskCount=0;
std::atomic<int> consCount=0;
Scheduler test;


void savetaskFile(int elem) {
    std::ofstream file("tasks.txt");
    if (file.is_open()) {
    for (int id = 0; id < elem; ++id) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> distr(0, 1);

        int type = distr(gen);  // Randomly assign type as Regular (0) or Irregular (1)
        double instructions = 0;
        double cpi = 0;

        if (type == Regular) {
            std::normal_distribution<double> distribution(0.51, 0.5);
            cpi = std::abs(distribution(gen));
            instructions = 200;
            
        } else {
            std::normal_distribution<double> distribution(0.51, 0.5);
            double random_value = std::abs(distribution(gen));
            cpi = std::abs(distribution(gen));
            instructions = random_value * 200;
        }

        Task task(id, static_cast<TaskType>(type), instructions, cpi);
        int typeStr = task.getType();
        file << id << ' ' << typeStr << ' ' << task.getInstructions() << task.getCPI() <<'\n';
    }
    } else {
        std::cerr << "Error: Could not open tasks.txt\n";
    }
    file.close();
}


void loadtaskFile(const std::string& pathTaskFile) {
    std::ifstream file(pathTaskFile);
    if (file.is_open()) {
        int id;
        int type;
        double instructions;
        double cpi;
        while (file >> id >> type >> instructions >> cpi) {
            TaskType taskType = type == 0 ? TaskType::Regular : TaskType::Irregular;

            taskBuffer.push(new Task(id, taskType, instructions, cpi));
            taskCount++;

            std::cout << "Task " << id << " " << taskType << " loaded\n";

        }
    } else {
        std::cerr << "Error: Could not open tasks.txt\n";
    }
    file.close();
}


void saveworkersFile(int consumers, int cpu) {
    std::ofstream file("workers.txt");

    if (file.is_open()) {
        for (int id = 0; id < consumers; ++id) {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::normal_distribution<double> distribution(0.1, 2);
            double ret = std::abs(distribution(gen));
            ConsumerType type;

            if (id < cpu) {
                type = ConsumerType::CPU;
            } else {
                type = ConsumerType::GPU;
            }

            double frequency = ret;

            if (type == ConsumerType::CPU) {
                ret *= 1.2;
            } else {
                ret *= 0.8;
            }

            Consumer cons(id, type, frequency);

            file << id << ' ' << static_cast<int>(cons.getType()) << ' ' << cons.getFreq() << '\n';
        }
    } else {
        std::cerr << "Error: Could not open workers.txt\n";
    }
    file.close();
}

void loadworkersFile(const std::string& pathWorkerFile) {
    std::ifstream file(pathWorkerFile);
    if (file.is_open()) {
        int id;
        int type; // Change the type to an integer.
        double frequency;
        while (file >> id >> type >> frequency) {
            consumerlist.push_back(Consumer(id, static_cast<ConsumerType>(type), frequency));
            consCount++;
            //std::cout << "Consumer " << consCount << " loaded\n";
        }
    } else {
        std::cerr << "Error: Could not open workers.txt\n";
    }
    file.close();
}


void producer(int id, int elem) {

    for (int i = 0; i < elem; ++i) {
        
        int taskId = globalTaskId++;  // Get the current ID and increment it
     

        taskBuffer.push(new Task(taskId, Regular, 0, 0));
        taskCount++;
            
        std::cout << "Producer" << id << ": " << taskId << std::endl;
                
    }


}


void scheduler(){

    int chunkSize = 3;

    test.startScheduling(taskCount, chunkSize, consumerlist, taskBuffer);

    test.setSchedulersFinished(true);

}


void consumer (int id){

    #ifdef D_LOL
                
    #endif

    //std::this_thread::sleep_for(std::chrono::milliseconds(5)); // Sleep to prevent race conditions
    

    Consumer& cons = consumerlist[id];

    int i = taskCount;
    int workload = cons.getWrkld(); 

    float flag = workload * 0.2;

    auto start = std::chrono::high_resolution_clock::now();

    while(!test.schedulerFinished() || !cons.isTaskBufferEmpty()) {

        if (!cons.isTaskBufferEmpty()){

            Task* taskPtr = cons.popTask();
                
            std::lock_guard<std::mutex> lock(printMutex); 
            std::cout << "Consumer " << cons.getId() << ": " ;

            float clk_frq = cons.getFreq();

            if (taskPtr != nullptr){

                std::this_thread::sleep_for(std::chrono::milliseconds(500));

                if (useProducer) {
                   taskPtr->run(clk_frq);
                } else {
                    taskPtr->runfromfile(clk_frq);

                    //ver o tempo desde que começou a consumir
                    auto now = std::chrono::high_resolution_clock::now();
                    std::chrono::duration<float> elapsed = now - start;
                    std::cout << "Elapsed time: " << elapsed.count() << " seconds\n";

                    //std::cout << taskPtr->getId() << " " << taskPtr->getType() << "\n";
                }  
            }
                
            delete taskPtr;
            
            --cons.wrkld;


            if (cons.wrkld < flag) {
                cons.setNeedMoreTasks(true);
            }

        }         
     
        
    } 

}


void usage() {
    std::cout << "Usage: program_name [-n num_elems num_consumers num_cpu num_producers | -f path_worker_file path_tasks_file | -cc num_consumers num_cpu path_tasks_file | -cp num_elems path_worker_file num_producers]\n";
}

int main(int argc, char* argv[]) {
    
    if (argc < 2) {
        std::cerr << "Error: No command provided.\n";
        usage();
        return 1;
    }

    std::string command = argv[1];
    if (command == "-n" && argc == 6) {
        int elem = std::stoi(argv[2]);
        int consumers = std::stoi(argv[3]);
        int cpu = std::stoi(argv[4]);
        int producers = std::stoi(argv[5]);

        if (cpu>consumers) {
            std::cerr << "Error: The number of consumers is less than the number of CPU consumers. The number of cpu's will be reduced from " << cpu << " to " << consumers << ".\n" << std::endl;
            cpu=consumers; 
        }

        saveworkersFile(consumers, cpu);
        loadworkersFile("workers.txt");
        useProducer=true;

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

        

        std::thread producerThreads[producers];

        std::thread consumerThreads[consumers];


        for (int i = 0; i < producers; ++i) {
            int workload = producer_workload + (i < producer_remainder ? 1 : 0);
            producerThreads[i] = std::thread(producer, i, workload);
        }

        std::thread schedulerThread(scheduler);
            

        for (int i = 0; i < consumers; ++i) {
            consumerThreads[i] = std::thread(consumer, i);
        }

        
        for (int i = 0; i < producers; i++) {
            producerThreads[i].join();
        }

        producersFinished = true;
        

        for (int i = 0; i < consumers; i++) {
            consumerThreads[i].join();
        }

        schedulerThread.join();
       

    } else if (command == "-f" && argc == 4) {
        std::string pathWorkerFile = argv[2];
        std::string pathTaskFile = argv[3];
        loadworkersFile(pathWorkerFile);
        loadtaskFile(pathTaskFile);

        int elem = taskCount;
        int consumers = consCount;


        int consumer_workload = (elem / consumers); 

        if (consumer_workload == 0) {
            std::cout << "WARNING: The number of consumers is greater than the number of elements to be produced. The number of consumers will be reduced from " << consumers << " to " << elem << ".\n" << std::endl;
            consumers = elem;
        } 


        std::thread consumerThreads[consumers];

        std::thread schedulerThread(scheduler);
            

        for (int i = 0; i < consumers; ++i) {
            consumerThreads[i] = std::thread(consumer, i);
        }


        producersFinished = true;
        

        for (int i = 0; i < consumers; i++) {
            consumerThreads[i].join();
        }

        schedulerThread.join();


    } else if (command == "-cc" && argc == 5) {
        int consumers = std::stoi(argv[2]);
        int cpu = std::stoi(argv[3]);
        std::string pathTaskFile = argv[4];

        if (cpu>consumers) {
            std::cerr << "Error: The number of consumers is less than the number of CPU consumers. The number of cpu's will be reduced from " << cpu << " to " << consumers << ".\n" << std::endl;
            cpu=consumers;
        }
        
        saveworkersFile(consumers, cpu);
        loadworkersFile("workers.txt");
        loadtaskFile(pathTaskFile);

        int elem = taskCount;

        int consumer_workload = (elem / consumers); 

        if (consumer_workload == 0) {
            std::cout << "WARNING: The number of consumers is greater than the number of elements to be produced. The number of consumers will be reduced from " << consumers << " to " << elem << ".\n" << std::endl;
            consumers = elem;
        } 


        std::thread consumerThreads[consumers];

        std::thread schedulerThread(scheduler);
            

        for (int i = 0; i < consumers; ++i) {
            consumerThreads[i] = std::thread(consumer, i);
        }


        producersFinished = true;


        for (int i = 0; i < consumers; i++) {
            consumerThreads[i].join();
        }

        schedulerThread.join();

    } else if (command == "-cp" && argc == 5) {
        int elem = std::stoi(argv[2]);
        std::string pathWorkerFile = argv[3];
        int producers = std::stoi(argv[4]);
        useProducer=true;
        loadworkersFile("workers.txt");

        int consumers = consCount;

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

        std::thread producerThreads[producers];

        std::thread consumerThreads[consumers];


        for (int i = 0; i < producers; ++i) {
            int workload = producer_workload + (i < producer_remainder ? 1 : 0);
            producerThreads[i] = std::thread(producer, i, workload);
        }

        std::thread schedulerThread(scheduler);
            

        for (int i = 0; i < consumers; ++i) {
            consumerThreads[i] = std::thread(consumer, i);
        }

        
        for (int i = 0; i < producers; i++) {
            producerThreads[i].join();
        }

        producersFinished = true;
        
              

        for (int i = 0; i < consumers; i++) {
            consumerThreads[i].join();
        }

        schedulerThread.join();
       
    } else if (command == "-help" && argc == 2) {
        usage();
    } else {
        std::cerr << "Error: Invalid command or incorrect number of arguments.\n";
        usage();
        return 1;
    }


    return 0;
}
