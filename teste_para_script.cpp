#include <iostream>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <unistd.h>
#include <random>
#include <atomic>

class Tasks {
public:
    int id;
    bool regular;

    int getId() const {
        return id;
    }

    // Constructor
    Tasks(int task_id) : id(task_id) {
        setRandomRegular();
    }

    void run() {
        if (regular) {
            std::cout << "Regular task " << id << "\n";
            usleep(200);
        } else {
            std::cout << "Irregular task " << id << ": ";
            std::random_device rd;
            std::mt19937 gen(rd());
            std::normal_distribution<double> distribution(0.51, 0.5);

            double random_value = std::abs(distribution(gen));
            usleep(static_cast<int>(random_value * 200));

            std::cout << random_value << "\n";
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

std::deque<Tasks> taskBuffer;
std::mutex mtx;
std::condition_variable cv;
int globalTaskId = 0;  // Global variable to store the task ID
std::mutex idMutex;    // Mutex to protect the increment operation for globalTaskId
std::mutex consumerMutex;
std::vector<std::deque<Tasks>> taskBufferConsumer;
std::condition_variable producer_cv;  // new condition variable for the scheduler
std::atomic<bool> producersFinished = 0;




void producer(int id, int elem) {
    for (int i = 0; i < elem; ++i) {
        
            std::unique_lock<std::mutex> lock(idMutex);
            int taskId = globalTaskId++;  // Get the current ID and increment it
            lock.unlock();

            std::unique_lock<std::mutex> taskLock(mtx);
            taskBuffer.push_back(Tasks(taskId));
            taskLock.unlock();
            
            std::cout << "Producer" << id << ": " << taskId << std::endl;
                    
    }
    // producer_cv.notify_one();


    //std::this_thread::sleep_for(std::chrono::microseconds(1500));

}

//2ms works fine. 1.5 parece ser o sweetspot
//está dar erro, está dar unlock antes de acabar os produtores

void scheduler(int consumers) {

    
     while (!producersFinished) {
        std::this_thread::sleep_for(std::chrono::microseconds(10)); // Sleep to prevent busy waiting
    }


    std::unique_lock<std::mutex> lock(mtx);

    while(!taskBuffer.empty())
    {

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

            Tasks task = taskBuffer.front();
            taskBuffer.pop_front();

            taskBufferConsumer[i].push_back(task);

            lock.unlock();
        }
            //std::this_thread::sleep_for(std::chrono::microseconds(1500)); // to synchronize outputs

    }    
    cv.notify_all();
    //std::cout << "UNLOCK" << std::endl;
    //cv.notify_all();
    }
}

void consumer(int id, int num_items) {
    int i = 0;

    while (i < num_items) {
        std::unique_lock<std::mutex> lock(consumerMutex);
        //std::cout << "UNLOCK" << std::endl;
        cv.wait(lock, [&] { return !taskBufferConsumer[id].empty(); });
        //std::cout << "UNLOCK" << std::endl;

        if (!taskBufferConsumer[id].empty()) {
            Tasks task = taskBufferConsumer[id].front();
            taskBufferConsumer[id].pop_front();
            std::cout << "Consumer" << id << ": ";
            
            task.run();
            
            lock.unlock();
            ++i;
        }
         
        
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

    //if (consumers < producers) {producers = consumers;} //quick fix


    int consumer_workload = (elem / consumers); 
    int producer_workload = (elem / producers);
    int producer_remainder = elem % producers;
    int consumer_remainder = elem % consumers;
    

    if (consumer_workload == 0) {
        std::cout << "WARNING: The number of consumers is greater than the number of elements to be produced. The number of consumers will be reduced from " << consumers << " to " << elem << ".\n" << std::endl;
        consumers = elem;
    } 
    if (producer_workload == 0) {
        std::cout << "WARNING: The number of producers is greater than the number of elements to be produced. The number of producers will be reduced from " << producers << " to " << elem << ".\n" << std::endl;
        producers = elem;
    }

    taskBufferConsumer.resize(consumers);

    std::thread producerThreads[producers];

    std::thread consumerThreads[consumers];


    // Create producer threads
    for (int i = 0; i < producers; ++i) {
        int workload = producer_workload + (i < producer_remainder ? 1 : 0);
        producerThreads[i] = std::thread(producer, i, workload);
    }

    std::thread schedulerThread(scheduler, consumers);

    // Create consumer threads
    for (int i = 0; i < consumers; ++i) {
        int workload = consumer_workload + (i < consumer_remainder ? 1 : 0);
        consumerThreads[i] = std::thread(consumer, i, workload);
    }

 

    for (int i = 0; i < producers; i++) {
        producerThreads[i].join();
    }

    producersFinished = 1;

    //producer acabava depois do scheduler começar então meti aqui a função notify_one em vez de estar no producer
    std::cout << "UNLOCK" << std::endl;
    producer_cv.notify_one();
    

 
    schedulerThread.join();

    for (int i = 0; i < consumers; i++) {
        consumerThreads[i].join();
    }

    return 0;
}
