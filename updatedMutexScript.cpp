
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
std::atomic<bool> producersFinished = false;
std::atomic<bool> schedulersFinished = false;
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


void loadtaskFile() {
    std::ifstream file("tasks.txt");
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

void loadworkersFile() {
    std::ifstream file("workers.txt");
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


void scheduler(int consumers) {
    
    while (!producersFinished) {
        std::this_thread::sleep_for(std::chrono::microseconds(10)); // Sleep to prevent race conditions
    }

    std::unique_lock<std::mutex> lock(mtx);

    while(!taskBuffer.empty()){

        producer_cv.wait(lock, []{ return !taskBuffer.empty(); }); 


        //std::cout << "LOCK" << std::endl;
            
        int totalTasks = taskBuffer.size();
        int consumer_workload = totalTasks / consumers;
        int consumer_remainder = totalTasks % consumers;



        lock.unlock();

        for (int i = 0; i < consumers; ++i) {
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

            

            //task.run(clk_frq); //se usar producer tem que ser assim, caso contrário tem que se usar 
            task.runfromfile(clk_frq);
            


                

            --workload;

            
        }

        lock.unlock();
    }
}

//antes  o consumer estava a fazer o mesmo calculo que o scheduler e recebi alogo da main quanto cada consumer iria consumir. Eu achei isso reduntdante então criei o *workload o scheduler mete lá as tasks que cada um tem que consumir baseado no seu ID e a consumer function vai buscar 
//ele tem na mesma o seu buffer com as tarefas e tem depois o workload com o numero de tarefas para consumir, eu tentei fazer tudo só com um buffer mas dava sempre erro ou deadlock então desisti e mudei para isto 

int main(int argc, char* argv[]) {
    if (argc != 5) {
        std::cerr << "Usage: " << argv[0] << " <num_consumers> <num_cpu> <num_producers> <num_elements>\n";
        return 1;
    }

    int consumers= std::stoi(argv[1]);
    int cpu = std::stoi(argv[2]);
    int producers= std::stoi(argv[3]);
    int elem = std::stoi(argv[4]);



    

    //saveworkersFile(consumers, cpu); // Save the workers to a file with the number of consumers and the number of CPU workers
    loadworkersFile();

 

    //savetaskFile(elem);
    loadtaskFile();

    elem = taskBuffer.size();



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


    //when loaded fromfiles i comment the producers

    //std::thread producerThreads[producers];


    std::thread consumerThreads[consumers];



    // for (int i = 0; i < producers; ++i) {
    //     int workload = producer_workload + (i < producer_remainder ? 1 : 0);
    //     producerThreads[i] = std::thread(producer, i, workload);
    // }



    std::thread schedulerThread(scheduler, consumers);
        

    for (int i = 0; i < consumers; ++i) {
        consumerThreads[i] = std::thread(consumer, i);
    }

 

    // for (int i = 0; i < producers; i++) {
    //     producerThreads[i].join();
    // }



    producersFinished = true;

    //producer acabava depois do scheduler começar então meti aqui a função notify_one em vez de estar no producer
    //std::cout << "UNLOCK" << std::endl;
    producer_cv.notify_one();
    
 
    schedulerThread.join();

    schedulersFinished=true;

    for (int i = 0; i < consumers; i++) {
        consumerThreads[i].join();
    }


    return 0;
}
