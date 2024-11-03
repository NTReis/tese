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

public:

bool schedulers_finished = false;

Scheduler(){
    }

int distributePredictedTasks(Consumer& cons, float controlPred, boost::lockfree::queue<Task*, boost::lockfree::fixed_sized<false>>& taskBuffer) {

    bool flag = true;
    
    cons.copyTasks();
    int j = 0;
    float tempo = 0.0;

    mtx.lock();

 

    for (j = 0; flag; ++j) {
        Task* taskPtr;
        if (taskBuffer.pop(taskPtr)) {
            float aux = taskPtr->getTemp();
            float new_tempo = tempo + (aux / cons.getFreq());
            if (new_tempo <= controlPred) {
                tempo = new_tempo;
                cons.pushTask(taskPtr);
            } else {
                taskBuffer.push(taskPtr);  // Put the task back
                flag = false; // Exceeds controlPred, break the loop
                //j--; 
                  
            }
        } else {
            flag = false;  // taskBuffer is empty, break the loop
        }
    }
      

    if (j > 0){
        std::cout << "Scheduler distributing " << j << " tasks to Consumer " << cons.getId()<< " with the time of: " << tempo << "\n" << std::endl;
    }
    mtx.unlock(); 

    cons.wrkld += j;
    
    
    if (cons.getNeedMoreTasks()){
        cons.setNeedMoreTasks(false);
    }

    

    return j;
}

float distributeFirstTask(Consumer& cons, int chunkSize, boost::lockfree::queue<Task*, boost::lockfree::fixed_sized<false>>& taskBuffer) {

    bool flag = true;

    float tempSum = 0;
    
    cons.copyTasks();
    cons.wrkld += chunkSize;

    mtx.lock();
    if (chunkSize > 0){
        std::cout << "Scheduler distributing " << chunkSize << " tasks to Consumer " << cons.getId() << std::endl;
    }
 

    for (int j = 0; j < chunkSize && flag; ++j) {
        Task* taskPtr;
        if (taskBuffer.pop(taskPtr)) {
            cons.pushTask(taskPtr);
            tempSum += taskPtr->getTemp();

                        
        } else {
            flag = false;
          // taskBuffer is empty, break the loop
        }
    }
    float result = tempSum/ cons.getFreq();

    std::cout << "Control time of " << result << " ms\n" << std::endl;

    mtx.unlock();   
    
    

    if (cons.getNeedMoreTasks()){
        cons.setNeedMoreTasks(false);
    }

    return result;
}

void distributeTasks(Consumer& cons, int chunkSize, boost::lockfree::queue<Task*, boost::lockfree::fixed_sized<false>>& taskBuffer) {


    bool flag = true;
    
    cons.copyTasks();
    cons.wrkld += chunkSize;

    mtx.lock();
    if (chunkSize > 0){
        std::cout << "\nScheduler distributing " << chunkSize << " tasks to Consumer " << cons.getId() << "\n" << std::endl;
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
    mtx.unlock();   
    
    
    if (cons.getNeedMoreTasks()){
        cons.setNeedMoreTasks(false);
    }
}

void startScheduling(int totalTasks, int chunkSize, std::vector<Consumer>& consumerlist, boost::lockfree::queue<Task*, boost::lockfree::fixed_sized<false>>& taskBuffer) {
    
    int totalConsumers = consumerlist.size();

    Consumer& cons = consumerlist[0];
    float controlPred = distributeFirstTask(cons, chunkSize, taskBuffer);
    totalTasks -= chunkSize;
    int restantes = 0;


    for (int i = 1; i < totalConsumers && totalTasks > 0; ++i) {
        Consumer& cons = consumerlist[i];
        
        restantes = distributePredictedTasks(cons, controlPred, taskBuffer);
        totalTasks -= restantes;


        
    } 

    while (!taskBuffer.empty() && totalTasks > -1) {
        for (int i = 0; i < totalConsumers; ++i) {
            Consumer& cons = consumerlist[i];
            if (cons.getNeedMoreTasks()) {
                int distributed = distributePredictedTasks(cons, controlPred, taskBuffer);
                totalTasks -= distributed;
                if (distributed == 0) {
                    cons.setNeedMoreTasks(false); //Neste momento o consumidor pede tasks quando tiver as restantes 20%, se quiser alterar tenho que mudar na função consumer no LockFree.cpp
                }
            }
        }
    }
}


void setSchedulersFinished(bool value){
    mtx.lock();
    schedulers_finished = value;
    mtx.unlock();
} 

bool schedulerFinished(){

    mtx.lock();

    bool value = schedulers_finished;

    mtx.unlock();

    return value;

}

};

#endif