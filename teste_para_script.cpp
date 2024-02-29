#include <iostream>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <unistd.h>
#include <random>

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

void producer(int id, int elem) {
    for (int i = 0; i < elem; ++i) {
        {
            std::unique_lock<std::mutex> lock(idMutex);
            int taskId = globalTaskId++;  // Get the current ID and increment it
            lock.unlock();

            std::unique_lock<std::mutex> taskLock(mtx);
            taskBuffer.push_back(Tasks(taskId));
            taskLock.unlock();
            cv.notify_one();
            std::cout << "Producer" << id << ": " << taskId << std::endl;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10)); // to synchronize outputs
    }

    cv.notify_all();
}

void consumer(int id, int num_items) {
    int i = 0;

    while (i < num_items) {
        std::unique_lock<std::mutex> lock(mtx);

        cv.wait(lock, [&] { return !taskBuffer.empty(); });

        if (!taskBuffer.empty()) {
            Tasks task = taskBuffer.front();
            taskBuffer.pop_front();
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
    } else if (producer_workload == 0) {
        std::cout << "WARNING: The number of producers is greater than the number of elements to be produced. The number of producers will be reduced from " << producers << " to " << elem << ".\n" << std::endl;
        producers = elem;
    }


    std::thread producerThreads[producers];

    std::thread consumerThreads[consumers];


    // Create producer threads
    for (int i = 0; i < producers; ++i) {
        int workload = producer_workload + (i < producer_remainder ? 1 : 0);
        producerThreads[i] = std::thread(producer, i, workload);
    }


    // Create consumer threads
    for (int i = 0; i < consumers; ++i) {
        int workload = consumer_workload + (i < consumer_remainder ? 1 : 0);
        consumerThreads[i] = std::thread(consumer, i, workload);
    }

    for (int i = 0; i < producers; i++) {
        producerThreads[i].join();
    }

    
    for (int i = 0; i < consumers; i++) {
        consumerThreads[i].join();
    }

    return 0;
}
