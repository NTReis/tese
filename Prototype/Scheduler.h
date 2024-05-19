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

class Scheduler{

private:

std::mutex mtx; // Mutex to protect the print operation

bool isTaskBufferEmpty(boost::lockfree::queue<Task*, boost::lockfree::fixed_sized<false>> taskBuffer) {
    return taskBuffer.empty();
}

public:

void distributeTasks(Consumer& cons, int chunkSize, boost::lockfree::queue<Task*, boost::lockfree::fixed_sized<false>>& taskBuffer) {
    
    cons.copyTasks();
    cons.wrkld += chunkSize;

    mtx.lock();
    if (chunkSize > 0){
        std::cout << "Scheduler distributing " << chunkSize << " tasks to Consumer " << cons.getId() << std::endl;
    }
    mtx.unlock(); 

    for (int j = 0; j < chunkSize; ++j) {
        Task* taskPtr;
        if (taskBuffer.pop(taskPtr)) {
            cons.pushTask(taskPtr);
                        
        } else {
            break;  // taskBuffer is empty, break the loop
        }
    }  
    
    
    if (cons.getNeedMoreTasks()){
            cons.setNeedMoreTasks(false);
    }
}

void start_scheduling(int totalTasks, int chunkSize, std::vector<Consumer>& consumerlist, boost::lockfree::queue<Task*, boost::lockfree::fixed_sized<false>>& taskBuffer) {
    
    int totalConsumers = consumerlist.size();


    for (int i = 0; i < totalConsumers; ++i) {
        Consumer& cons = consumerlist[i];
        if (totalTasks >= chunkSize){
            distributeTasks(cons, chunkSize, taskBuffer);
            totalTasks -= chunkSize;
        } else {
            distributeTasks(cons, totalTasks, taskBuffer);
            totalTasks = 0;
        }
    } 

    while (!taskBuffer.empty() && totalTasks > -1){  // Check if there are tasks left
        for (int i = 0; i < totalConsumers; ++i){
            Consumer& cons = consumerlist[i];
            if (cons.getNeedMoreTasks()  && totalTasks >= chunkSize){
                distributeTasks(cons, chunkSize, taskBuffer);
                totalTasks -= chunkSize;
            } else if (cons.getNeedMoreTasks() && totalTasks < chunkSize){
                distributeTasks(cons, totalTasks, taskBuffer);
                totalTasks = 0;
            }  if (cons.getNeedMoreTasks()) {
                cons.setNeedMoreTasks(false);
            }
        }
        
    }

}


};

#endif
