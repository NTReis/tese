#ifndef SCHEDULERPRED_H
#define SCHEDULERPRED_H

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

std::mutex mtx;
bool isTaskBufferEmpty(boost::lockfree::queue<Task*, boost::lockfree::fixed_sized<false>> taskBuffer) {
    return taskBuffer.empty();
}

std::atomic<int> tasks2schedule{0};

public:

std::atomic<bool> schedulers_finished{false};

Scheduler(){
    }

int distributePredictedTasks(Consumer& cons, float controlPred, boost::lockfree::queue<Task*, boost::lockfree::fixed_sized<false>>& taskBuffer) {

    bool flag = true;
    
    cons.copyTasks();
    int j = 0;
    float tempo = 0.0;

    

 
    mtx.lock();
    while(flag) {
        Task* taskPtr;
        if (taskBuffer.pop(taskPtr)) {
            float aux = taskPtr->getPred();
            float new_tempo = tempo + (aux / cons.getFreq());
            if (new_tempo <= controlPred) {
                tempo = new_tempo;
                cons.pushTask(taskPtr);
                j++;
            } else {
                taskBuffer.push(taskPtr);  // Put the task back
                flag = false; // Exceeds controlPred, break the loop
            }
        } else {
            flag = false;  // taskBuffer is empty, break the loop
        }
    }
      

    if (j > 0){
        std::cout << "\nScheduler distributing " << j << " tasks to Consumer " << cons.getId()<< " with the time of: " << tempo << "\n" << "tasks left:" << tasks2schedule << "\n" << std::endl;
    }

    

    cons.wrkld += j;
    mtx.unlock(); 
    
    
    if (cons.getNeedMoreTasks()){
        cons.setNeedMoreTasks(false);
    }

    
    

    return j;
}

float distributeFirstTasks(Consumer& cons, int chunkSize, boost::lockfree::queue<Task*, boost::lockfree::fixed_sized<false>>& taskBuffer) {

    bool flag = true;

    float tempSum = 0;
    
    cons.copyTasks();
    cons.wrkld += chunkSize;

    mtx.lock();
    if (chunkSize > 0){
        std::cout << "Scheduler distributing " << chunkSize << " tasks to Consumer " << cons.getId() << "tasks left:" << tasks2schedule << "\n" << std::endl;
    }
 

    for (int j = 0; j < chunkSize && flag; ++j) {
        Task* taskPtr;
        if (taskBuffer.pop(taskPtr)) {
            cons.pushTask(taskPtr);
            tempSum += taskPtr->getPred();

                        
        } else {
            flag = false;
          // taskBuffer is empty, break the loop
        }
    }
    float result = tempSum/ cons.getFreq();
    
    mtx.unlock();  

    std::cout << "Control time of " << result << " ms\n" << std::endl;

    if (cons.getNeedMoreTasks()){
        cons.setNeedMoreTasks(false);
    }

     
    
    

    return result;
}


void startScheduling(int totalTasks, int chunkSize, std::vector<Consumer>& consumerlist, boost::lockfree::queue<Task*, boost::lockfree::fixed_sized<false>>& taskBuffer) {
    
    int totalConsumers = consumerlist.size();

    tasks2schedule = totalTasks;

    Consumer& cons = consumerlist[0];
    float controlPred = distributeFirstTasks(cons, chunkSize, taskBuffer);
    tasks2schedule -= chunkSize;
    int restantes = 0;


    for (int i = 0; i < totalConsumers && tasks2schedule > 0; ++i) {
        Consumer& cons = consumerlist[i];
        
        restantes = distributePredictedTasks(cons, controlPred, taskBuffer);
        tasks2schedule = std::max(0, tasks2schedule - restantes);
        
    } 
    std::cout << "Tasks left to schedule: " << tasks2schedule << ", Queue empty: " << taskBuffer.empty() << std::endl;

    while (tasks2schedule > 0 && !taskBuffer.empty()){
        
        for (int i = 0; i < totalConsumers; ++i) {
            Consumer& cons = consumerlist[i];
            if (cons.getNeedMoreTasks()) {

                //std::cout << "hey";

                //std::cout << "\nConsumer " << cons.getId() << " needs more tasks.\n" << std::endl;

                int distributed = distributePredictedTasks(cons, controlPred, taskBuffer);
                tasks2schedule = std::max(0, tasks2schedule - distributed);

                // // Debug print after distributing tasks
                // std::cout << "Distributed " << distributed << " tasks to Consumer " << cons.getId() 
                // << ". Tasks left to schedule: " << tasks2schedule << std::endl;



                if (distributed == 0) {
                    cons.setNeedMoreTasks(false); //Neste momento o consumidor pede tasks quando tiver as restantes 20%, se quiser alterar tenho que mudar na função consumer no LockFree.cpp
                }
            }
        }
    }
}


void setSchedulersFinished(bool value){
    std::lock_guard<std::mutex> lock(mtx);
    schedulers_finished = value;
    
} 

bool schedulerFinished(){
    std::lock_guard<std::mutex> lock(mtx);
    

    bool value = schedulers_finished;

    

    return value;

}

};

#endif
