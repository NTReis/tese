#ifndef SCHEDULERWS_H
#define SCHEDULERWS_H

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

class Scheduler {

private:

    std::mutex mtx; // Mutex to protect shared operations
    std::atomic<int> totalRemainingTasks;
    std::condition_variable cv;
    bool schedulerWorking = false;

public:

    bool schedulers_finished = false;

    Scheduler() {}

    void distributeTasks(Consumer& cons, int chunkSize, boost::lockfree::queue<Task*, boost::lockfree::fixed_sized<false>>& taskBuffer) {

        cons.copyTasks();
        cons.wrkld += chunkSize;

        {
            //std::lock_guard<std::mutex> guard(mtx);
            mtx.lock();
            schedulerWorking = true; // Scheduler starts working
            cv.notify_all();         // sleep
            mtx.unlock();
        }

        if (chunkSize > 0) {
            std::cout << "Scheduler distributing " << chunkSize << " tasks to Consumer " << cons.getId() << std::endl;
        }

        mtx.lock();
        bool flag = true;
        for (int j = 0; j < chunkSize && flag; ++j) {
            Task* taskPtr;
            if (taskBuffer.pop(taskPtr)) {
                cons.pushTask(taskPtr);
            } else {
                flag = false; 
            }
        }
        mtx.unlock();

        {
            mtx.lock();
            schedulerWorking = false; // Scheduler done working
            cv.notify_all();          // resume
            mtx.unlock();
        }
    }

    void redistributeTasks(std::vector<Consumer>& consumerList) {
        Consumer* maxTaskConsumer = nullptr;
        Consumer* needyConsumer = nullptr;
        int maxTasks = 0;

        for (Consumer& cons : consumerList) {

            if (cons.getWrkld() < 1) {
                cons.setNeedMoreTasks(true);
            }

            if (cons.getNeedMoreTasks()) {
                needyConsumer = &cons;
            }

            int taskCount = cons.getTaskCount(); 

            if (taskCount > maxTasks && cons.getWrkld() > 1) {
                maxTasks = taskCount;
                maxTaskConsumer = &cons;
            } 
        }

        if (needyConsumer && maxTaskConsumer && maxTaskConsumer != needyConsumer) {
            // std::lock_guard<std::mutex> guard(mtx);
            mtx.lock();

            schedulerWorking = true; // Scheduler working, consumers sleep
            cv.notify_all();
            
            int tasksToSteal = std::min(maxTasks / 2, maxTaskConsumer->getWrkld() - 1);
            if (tasksToSteal > maxTaskConsumer->getWrkld()) {
                tasksToSteal = maxTaskConsumer->getWrkld();
            }

            if (tasksToSteal > 0) {
                std::cout << "STEALING: Consumer " << needyConsumer->getId() << " stealing " << tasksToSteal << " tasks from Consumer " << maxTaskConsumer->getId() << std::endl;

                for (int i = 0; i < tasksToSteal; ++i) {
                    Task* taskPtr = maxTaskConsumer->popTask();
                    if (taskPtr) {
                        needyConsumer->pushTask(taskPtr);
                    }
                }

                needyConsumer->wrkld += tasksToSteal;
                maxTaskConsumer->wrkld -= tasksToSteal;
                needyConsumer->setNeedMoreTasks(false);

                std::cout << "UPDATED WORKLOAD: Consumer " << needyConsumer->getId() << " -> " << needyConsumer->getWrkld() << " and Consumer " << maxTaskConsumer->getId() << " -> " << maxTaskConsumer->getWrkld() << std::endl;
            }
            mtx.unlock();

            schedulerWorking = false; // Scheduler done, consumers resume
            cv.notify_all();
        }
    }

    void startScheduling(int totalTasks, int nothingBg, std::vector<Consumer>& consumerList, boost::lockfree::queue<Task*, boost::lockfree::fixed_sized<false>>& taskBuffer) {
        int totalConsumers = consumerList.size();

        int chunkSize = totalTasks / totalConsumers;
        int resto = totalTasks % totalConsumers;

        for (int i = 0; i < totalConsumers; ++i) {
            Consumer& cons = consumerList[i];

            if ((i + 1) != totalConsumers) {
                distributeTasks(cons, chunkSize, taskBuffer);
                totalTasks -= chunkSize;
            } else {
                int restante = chunkSize + resto;
                distributeTasks(cons, restante, taskBuffer);
                totalTasks = 0;
            }
        }

        // Work-stealing 
        bool allConsumersFinished = false;
        bool stealingOccurred = false;
        while (!allConsumersFinished) {
            allConsumersFinished = true;
            int flag = 0;

            for (int i = 0; i < totalConsumers; ++i) {
                Consumer& cons = consumerList[i];

                if (cons.getWrkld() == 0) {
                    flag++;
                    continue;
                }

                if (cons.getNeedMoreTasks()) {
                    redistributeTasks(consumerList);
                    stealingOccurred = true;  // Mark that stealing happened
                    cons.setNeedMoreTasks(false); // Reset the main flag
                }

                if (cons.getWrkld() != 0) {
                    allConsumersFinished = false;
                }
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(10));

            if (flag == totalConsumers) {
                if (stealingOccurred) {
                    std::cout << "All consumers have finished their tasks. Work stealing was performed." << std::endl;
                } else {
                    std::cout << "All consumers have finished their tasks. No stealing required." << std::endl;
                }
                allConsumersFinished = true;
            }   
        }
     
    }
    
    void setSchedulersFinished(bool value) {
        std::lock_guard<std::mutex> guard(mtx);
        schedulers_finished = value;
    }

    bool schedulerFinished() {
        std::lock_guard<std::mutex> guard(mtx);
        return schedulers_finished;
    }

    bool isSchedulerWorking() {
        std::lock_guard<std::mutex> guard(mtx);
        return schedulerWorking;
    }

    void startStreamingScheduling(int numProducers, int totalTasks, int chunkSize, std::vector<Consumer>& consumerlist, boost::lockfree::queue<Task*, boost::lockfree::fixed_sized<false>>& taskBuffer){};
};

#endif
