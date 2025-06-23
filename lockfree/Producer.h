#ifndef PRODUCER_H
#define PRODUCER_H

#include <iostream>
#include <random>
#include <atomic>
#include "boost/lockfree/queue.hpp"
#include <thread>
#include <chrono>
#include <mutex>
#include <condition_variable>
#include "Task.h"
#include <iostream>
#include <fstream>
#include <sstream>


// Global variables for task tracking
extern std::atomic<int> taskCount;
extern std::atomic<int> globalTaskId;
extern std::mutex idMutex, printMutex;
extern std::condition_variable producer_cv;
extern std::atomic<int> activeProducers;
extern boost::lockfree::queue<Task*, boost::lockfree::fixed_sized<false>> taskBuffer;

class Producer {

private:
    int delay;   // Delay in milliseconds before creating each task
    int limit;   // Maximum number of tasks to create
    int id;



public:
    Producer(int i, int d, int l)
        : id(i), delay(d), limit(l) {} 


    void produceSingleTask() {


        if (delay > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(delay));
        }


        int taskId;
        {
            std::lock_guard<std::mutex> lock(idMutex);
            taskId = globalTaskId++;
        }

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> distr(0, 1);
        int type = distr(gen);

        float instructions = 10;
        float cpi = 10;

        if (type == 0) {
            std::normal_distribution<float> distribution(0.51, 0.5);
            cpi = std::abs(distribution(gen));
            instructions = 200;
        } else {
            std::normal_distribution<float> distribution(0.51, 0.5);
            float random_value = std::abs(distribution(gen));
            cpi = std::abs(distribution(gen));
            instructions = random_value * 200;
        }

        TaskType taskType = (type == 0) ? TaskType::Regular : TaskType::Irregular;
        //Task* newTask = new Task(taskId, taskType, instructions, cpi);

        printMutex.lock();
            std::cout << "Producer " << id <<": Task " << taskId << " created after " << delay << " ms delay" << std::endl;
            
            savetaskFile(taskId, taskType, instructions, cpi);
        printMutex.unlock();



        while (!taskBuffer.push(new Task(taskId, taskType, instructions, cpi))) {
            std::this_thread::yield();
        }


        taskCount++;
    }

    // Add a method to produce N tasks
    void produceNTasks(int n) {
        for (int i = 0; i < n; ++i) {
            produceSingleTask();
        }

    }

    void savetaskFile(int id, TaskType type, float instructions, float cpi) {
    //std::lock_guard<std::mutex> lock(fileMutex);

    std::ofstream file("tasksProduced.txt", std::ios::app);
    if (!file.is_open()) {
        std::cerr << "Failed to open tasks.txt" << std::endl;
        return;
    }

    file << id << ' ' << static_cast<int>(type) << ' ' << instructions << ' ' << cpi << '\n';

    file.close();
}

};


#endif // PRODUCER_H
