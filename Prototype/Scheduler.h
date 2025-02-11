#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <iostream>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <unistd.h>
#include <random>
#include <atomic>
#include "boost/lockfree/queue.hpp"
#include <boost/lockfree/spsc_queue.hpp>
#include <fstream>
#include "Consumer.h"
#include "Task.h"
#include "Producer.h"

class Scheduler {
private:
    std::mutex mtx; // Mutex to protect the print operation
    std::atomic<int> tasksProduced{0};
    std::atomic<int> tasksScheduled{0};
    std::atomic<bool> producersActive{true};
    std::condition_variable cv;
    std::mutex cvMutex;
    int totalExpectedTasks;
    
    void producerThread(int producerId, int delay, int workload) {
        auto producer = std::make_unique<Producer>(producerId, delay, workload);
        
        for (int i = 0; i < workload && producersActive; ++i) {
            producer->produceSingleTask();
            tasksProduced++;
            
            // Notify scheduler that new tasks are available
            cv.notify_one();
        }
        
        {
            std::lock_guard<std::mutex> lock(mtx);
            std::cout << "Producer " << producerId << " finished." << std::endl;
        }
    }

public:
    bool schedulers_finished = false;

    Scheduler() : totalExpectedTasks(0) {}

    void distributeTasks(Consumer& cons, int chunkSize, boost::lockfree::queue<Task*, boost::lockfree::fixed_sized<false>>& taskBuffer) {
        bool flag = true;
        int tasksDistributed = 0;
        
        cons.copyTasks();
        
        mtx.lock();
        if (chunkSize > 0) {
            std::cout << "Scheduler distributing up to " << chunkSize << " tasks to Consumer " << cons.getId() << "\n" << std::endl;
        }
        mtx.unlock();

        for (int j = 0; j < chunkSize && flag; ++j) {
            Task* taskPtr;
            if (taskBuffer.pop(taskPtr)) {
                cons.pushTask(taskPtr);
                tasksDistributed++;
                tasksScheduled++;
            } else {
                flag = false;
            }
        }
        
        if (tasksDistributed > 0) {
            cons.wrkld += tasksDistributed;
        }
        
        if (cons.getNeedMoreTasks()) {
            cons.setNeedMoreTasks(false);
        }
    }

    void startStreamingScheduling(int numProducers, int totalTasks, int chunkSize, std::vector<Consumer>& consumerlist, boost::lockfree::queue<Task*, boost::lockfree::fixed_sized<false>>& taskBuffer) {
        
        totalExpectedTasks = totalTasks;
        tasksProduced = 0;
        tasksScheduled = 0;
        
        // Start producer threads
        std::vector<std::thread> producerThreads;
        int baseWorkload = totalTasks / numProducers;
        int remainder = totalTasks % numProducers;
        
        for (int i = 0; i < numProducers; ++i) {
            int workload = baseWorkload + (i < remainder ? 1 : 0);
            producerThreads.push_back(std::thread(&Scheduler::producerThread, this, i, 1, workload));
        }

        int totalConsumers = consumerlist.size();

        // Continue scheduling while we haven't scheduled all tasks
        while (tasksScheduled < totalExpectedTasks) {
            // Wait for new tasks
            {
                std::unique_lock<std::mutex> lock(cvMutex);
                cv.wait_for(lock, std::chrono::milliseconds(100),
                           [this, &taskBuffer]() { 
                               return !taskBuffer.empty() || tasksScheduled >= totalExpectedTasks; 
                           });
            }

            // Schedule available tasks
            for (int i = 0; i < totalConsumers && tasksScheduled < totalExpectedTasks; ++i) {
                Consumer& cons = consumerlist[i];
                if (cons.getNeedMoreTasks() || cons.getWrkld() == 0) {
                    int remainingTasks = totalExpectedTasks - tasksScheduled;
                    int currentChunkSize = std::min(chunkSize, remainingTasks);
                    if (currentChunkSize > 0) {
                        distributeTasks(cons, currentChunkSize, taskBuffer);
                    }
                }
            }
        }

        // Clean up producer threads
        producersActive = false;
        for (auto& thread : producerThreads) {
            if (thread.joinable()) {
                thread.join();
            }
        }
        
        setSchedulersFinished(true);
    }

    void setSchedulersFinished(bool value) {
        std::lock_guard<std::mutex> lock(mtx);
        schedulers_finished = value;
    }

    bool schedulerFinished() {
        std::lock_guard<std::mutex> lock(mtx);
        return schedulers_finished;
    }
};

#endif