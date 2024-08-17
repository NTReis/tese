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

class Scheduler{

private:

std::mutex mtx; // Mutex to protect the print operation

bool isTaskBufferEmpty(boost::lockfree::queue<Task*, boost::lockfree::fixed_sized<false>> taskBuffer) {
    return taskBuffer.empty();

}

public:

bool schedulers_finished = false;

Scheduler(){
    }


void distributeTasks(Consumer& cons, int chunkSize, boost::lockfree::queue<Task*, boost::lockfree::fixed_sized<false>>& taskBuffer) {

    bool flag = true;
    
    cons.copyTasks();
    cons.wrkld += chunkSize;

    //mtx.lock();
    std::lock_guard<std::mutex> guard(mtx);

    if (chunkSize > 0){
        std::cout << "Scheduler distributing " << chunkSize << " tasks to Consumer " << cons.getId() << std::endl;
    }
 

    for (int j = 0; j < chunkSize && flag; ++j) {
        Task* taskPtr;
        if (taskBuffer.pop(taskPtr)) {
            cons.pushTask(taskPtr);
                        
        } else {
            flag = false;
          // taskBuffer is empty, break the loop
        }
    }
    //mtx.unlock();   

}

void redistributeTasks(std::vector<Consumer>& consumerList) {
    Consumer* maxTaskConsumer = nullptr;
    Consumer* needyConsumer = nullptr;
    int maxTasks = 0;

    for (Consumer& cons : consumerList) {
        if (cons.getNeedMoreTasks()) {
            needyConsumer = &cons;
        }

        int taskCount = cons.getTaskCount();
        if (taskCount > maxTasks) {
            maxTasks = taskCount;
            maxTaskConsumer = &cons;
        }
    }

    int tasksToSteal = maxTasks / 2; //rouba metade das tasks


    std::lock_guard<std::mutex> guard(mtx);

    //mtx.lock();
    std::cout << "STEALING: Consumer " << needyConsumer->getId() << " stealing " << tasksToSteal << " tasks from Consumer " << maxTaskConsumer->getId() << std::endl;

    if (needyConsumer && maxTaskConsumer && maxTaskConsumer != needyConsumer) {
         // rouba metade das tasks
        for (int i = 0; i < tasksToSteal; ++i) {
            Task* taskPtr = maxTaskConsumer->popTask();
            if (taskPtr) {
                needyConsumer->pushTask(taskPtr);
            }
        }
    }
    //maxTaskConsumer->copyTasks();
    //needyConsumer->copyTasks();
    needyConsumer->wrkld += tasksToSteal;
    maxTaskConsumer->wrkld -= tasksToSteal;

    //mtx.unlock();
}

void startScheduling(int totalTasks, std::vector<Consumer>& consumerList, boost::lockfree::queue<Task*, boost::lockfree::fixed_sized<false>>& taskBuffer) {
    
    
    int totalConsumers = consumerList.size();

    int chunkSize = totalTasks/totalConsumers;
    int resto = totalTasks%totalConsumers;

    for (int i = 0; i < totalConsumers; ++i) {
        Consumer& cons = consumerList[i];

        if ((i+1)!=totalConsumers){
            distributeTasks(cons, chunkSize, taskBuffer);
            totalTasks -= chunkSize;

        } else {
            int restante = chunkSize+resto;
            distributeTasks(cons, restante, taskBuffer);
            totalTasks = 0;
        }
    } 

// Work-stealing phase
    bool allConsumersFinished = false;
    while (!allConsumersFinished) {
        allConsumersFinished = true;        

        for (int i = 0; i < totalConsumers; ++i) {
            Consumer& cons = consumerList[i];

            if (cons.getNeedMoreTasks()) {
                redistributeTasks(consumerList);
                cons.setNeedMoreTasks(false); // Reset the flag
            }


            // Check if this consumer has finished its tasks
            if (!cons.getWrkld()==0) {
                allConsumersFinished = false;
            }
        }
        
        if (allConsumersFinished) {
            std::cout << "All consumers have finished their tasks. No stealing required." << std::endl;
            break;
        }
    }
}


void setSchedulersFinished(bool value){
    std::lock_guard<std::mutex> guard(mtx);

    //mtx.lock();
    schedulers_finished = value;
    //mtx.unlock();
}

bool schedulerFinished(){
    std::lock_guard<std::mutex> guard(mtx);

    //mtx.lock();

    bool value = schedulers_finished;

    //mtx.unlock();

    return value;

}

};

#endif
