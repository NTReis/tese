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
std::deque<Consumer> consumerlist;
std::condition_variable cv;
std::condition_variable producer_cv;  // new condition variable for the scheduler
std::atomic<bool> producersFinished=false;
std::atomic<bool> schedulersFinished=false;
std::atomic<bool> useProducer=false;
int globalTaskId = 0;  // Global variable to store the task ID


void savetaskFile(int elem) {
    std::ofstream file("tasks.txt");
    if (file.is_open()) {
    for (int id = 0; id < elem; ++id) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> distr(0, 1);

        int type = distr(gen);  // Randomly assign type as Regular (0) or Irregular (1)
        double duration = 0;

        if (type == Regular) {
            duration = 200;
        } else {
            std::normal_distribution<double> distribution(0.51, 0.5);
            double random_value = std::abs(distribution(gen));
            duration = random_value * 200;
        }

        Task task(id, static_cast<TaskType>(type), duration);
        int typeStr = task.getType();
        file << id << ' ' << typeStr << ' ' << task.getDuration() << '\n';
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
        double duration;
        while (file >> id >> type >> duration) {
            type = type == 0 ? Regular : Irregular;

            taskBuffer.push_back(Task(id, static_cast<TaskType>(type), duration));

            std::cout << "Task " << id << " loaded\n";
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
            //std::cout << "Consumer " << id << " loaded\n";
        }
    } else {
        std::cerr << "Error: Could not open workers.txt\n";
    }
    file.close();
}


void producer(int id, int elem) {

    for (int i = 0; i < elem; ++i) {
        
            std::unique_lock<std::mutex> lock(idMutex);
            int taskId = globalTaskId++;  // Get the current ID and increment it

            lock.unlock();

            std::unique_lock<std::mutex> taskLock(mtx);
            taskBuffer.push_back(Task(taskId, Regular, 0));
            taskLock.unlock();
            
            std::cout << "Producer" << id << ": " << taskId << std::endl;
                    
    }


}


void scheduler(int totalConsumers) {
    
    while (!producersFinished) {
        std::this_thread::sleep_for(std::chrono::microseconds(10)); // Sleep to prevent race conditions
    }

    std::unique_lock<std::mutex> lock(mtx);

    while(!taskBuffer.empty()){

        producer_cv.wait(lock, []{ return !taskBuffer.empty(); }); 


        //std::cout << "LOCK" << std::endl;
            
        int totalTasks = taskBuffer.size();
        //int totalConsumers = consumerlist.size();
        int consumer_workload = totalTasks / totalConsumers;
        int consumer_remainder = totalTasks % totalConsumers;



        lock.unlock();


        for (int i = 0; i < totalConsumers; ++i) {
            int sharedtask = consumer_workload + (i < consumer_remainder ? 1 : 0);

            Consumer& cons = consumerlist[i];

            std::cout << "Scheduler distributing " << sharedtask << " tasks to Consumer" << i << std::endl;

            for (int j = 0; j < sharedtask && !taskBuffer.empty(); ++j) {
                    
                std::unique_lock<std::mutex> lock(consumerMutex);
                producer_cv.wait(lock, [&] { return !taskBuffer.empty(); });

                Task task = taskBuffer.front();
                taskBuffer.pop_front();

                cons.taskBufferConsumer.push_back(task);

                //std::cout << "Size of taskBufferConsumer for Consumer " << i << ": " << cons.taskBufferConsumer.size() << std::endl;

                lock.unlock();

                std::unique_lock<std::mutex> workloadlock(workloadMutex);

                cons.wrkld = sharedtask;
              

                workloadlock.unlock();
                
            }

        } 

        cv.notify_all();

    }
}



void consumer(int id) {

    while (!schedulersFinished) {
        std::this_thread::sleep_for(std::chrono::microseconds(10)); // Sleep to prevent race conditions
    }

    Consumer cons = consumerlist[id];
    int workload = cons.wrkld;
    //std::cout << workload;
    
    while (workload > 0) {
        std::unique_lock<std::mutex> lock(consumerMutex);

        cv.wait(lock, [&] { return !cons.taskBufferConsumer.empty(); });

        if (!cons.taskBufferConsumer.empty() && workload > 0) {

            Task task = cons.taskBufferConsumer.front();

            cons.taskBufferConsumer.pop_front();
            
            std::cout << "Consumer" << id << ": " ;

            // if (cons.getType() == 0) {

            //     std::cout << "CPU: " << std::endl;
            // } else { 
            //     std::cout << "GPU: " << std::endl;
            // }


            double clk_frq = cons.getFreq();

            if (useProducer==true) {
                task.run(clk_frq); //se usar producer tem que ser assim, caso contrário tem que se usar 
            } else {
                task.runfromfile(clk_frq);
            }

            

            --workload;
            
        }

        lock.unlock();
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

        std::thread schedulerThread(scheduler, consumers);
            

        for (int i = 0; i < consumers; ++i) {
            consumerThreads[i] = std::thread(consumer, i);
        }

        
        for (int i = 0; i < producers; i++) {
            producerThreads[i].join();
        }

        producersFinished = true;

        producer_cv.notify_one();
        
        schedulerThread.join();

        schedulersFinished=true;

        for (int i = 0; i < consumers; i++) {
            consumerThreads[i].join();
        }
       

    } else if (command == "-f" && argc == 4) {
        std::string pathWorkerFile = argv[2];
        std::string pathTaskFile = argv[3];
        loadworkersFile(pathWorkerFile);
        loadtaskFile(pathTaskFile);

        int elem = taskBuffer.size();
        int consumers = consumerlist.size();


        int consumer_workload = (elem / consumers); 

        if (consumer_workload == 0) {
            std::cout << "WARNING: The number of consumers is greater than the number of elements to be produced. The number of consumers will be reduced from " << consumers << " to " << elem << ".\n" << std::endl;
            consumers = elem;
        } 


        std::thread consumerThreads[consumers];

        std::thread schedulerThread(scheduler, consumers);
            

        for (int i = 0; i < consumers; ++i) {
            consumerThreads[i] = std::thread(consumer, i);
        }


        producersFinished = true;

        producer_cv.notify_one();
            
        schedulerThread.join();

        schedulersFinished=true;

        for (int i = 0; i < consumers; i++) {
            consumerThreads[i].join();
        }


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

        int elem = taskBuffer.size();

        int consumer_workload = (elem / consumers); 

        if (consumer_workload == 0) {
            std::cout << "WARNING: The number of consumers is greater than the number of elements to be produced. The number of consumers will be reduced from " << consumers << " to " << elem << ".\n" << std::endl;
            consumers = elem;
        } 


        std::thread consumerThreads[consumers];

        std::thread schedulerThread(scheduler, consumers);
            

        for (int i = 0; i < consumers; ++i) {
            consumerThreads[i] = std::thread(consumer, i);
        }


        producersFinished = true;

        producer_cv.notify_one();
            
        schedulerThread.join();

        schedulersFinished=true;

        for (int i = 0; i < consumers; i++) {
            consumerThreads[i].join();
        }

    } else if (command == "-cp" && argc == 5) {
        int elem = std::stoi(argv[2]);
        std::string pathWorkerFile = argv[3];
        int producers = std::stoi(argv[4]);
        useProducer=true;
        loadworkersFile("workers.txt");

        int consumers = consumerlist.size();

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

        std::thread schedulerThread(scheduler, consumers);
            

        for (int i = 0; i < consumers; ++i) {
            consumerThreads[i] = std::thread(consumer, i);
        }

        
        for (int i = 0; i < producers; i++) {
            producerThreads[i].join();
        }

        producersFinished = true;

        producer_cv.notify_one();
        
        schedulerThread.join();

        schedulersFinished=true;

        for (int i = 0; i < consumers; i++) {
            consumerThreads[i].join();
        }
       
    } else if (command == "-help" && argc == 2) {
        usage();
    } else {
        std::cerr << "Error: Invalid command or incorrect number of arguments.\n";
        usage();
        return 1;
    }


    return 0;
}
