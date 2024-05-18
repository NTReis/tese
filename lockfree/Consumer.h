#ifndef CONSUMER_H
#define CONSUMER_H

#include <iostream>
#include <deque>
#include <random>
#include <fstream>
#include <cmath>
#include <boost/lockfree/spsc_queue.hpp>
#include "Task.h" 
#include <vector>
#include "SPSCQueue.h"
#include <mutex>


enum ConsumerType {
    CPU,
    GPU
};

class Consumer {

private:
    //rigtorp::SPSCQueue<Task*> taskBufferConsumer{128};

    boost::lockfree::spsc_queue<Task*> taskBufferConsumer{128};

    std::vector<Task*> taskBufferConsumerCopy;

    bool needMoreTasks = false;
    std::mutex mtx;



public:

    void copyTasks(){

        int size = taskBufferConsumer.read_available();
        Task* task;
        for (int i = 0; i<size; i++){
            if (taskBufferConsumer.pop(task))
            taskBufferConsumerCopy.push_back(task);
            taskBufferConsumer.push(task);
        }

    }

    void setNeedMoreTasks(bool value) {
        mtx.lock();
        needMoreTasks = value;
        mtx.unlock();
    }

    bool getNeedMoreTasks() {
        mtx.lock();
        bool value = needMoreTasks;
        mtx.unlock();
        return value;
    }


    ConsumerType type;
    int id;
    double frequency;
    int wrkld;

    ConsumerType getType() const {
        return type;
    }

    int getId() const {
        return id;
    }

    double getFreq() const {
        return frequency;
    }

    int getWrkld() const {
        return wrkld;
    }



    // Construtor
    Consumer() = default;

    // Destructor Lockfree
    ~Consumer() {
        Task* task;
        
        while (taskBufferConsumer.pop(task)) {
            delete task; 
        }
    }


//     RIGTORP
//     ~Consumer() {
//     while (!taskBufferConsumer.empty()) {
//         Task** taskPtr = taskBufferConsumer.front();
//         taskBufferConsumer.pop();
//         delete *taskPtr; 
//     }
//    }
        

    Consumer& operator=(const Consumer& other) = default;

    // Construtor de cópia
    //Consumer(const Consumer& other) = default; 


    std::vector<Task*> getTaskBufferConsumerCopy() const {
        return taskBufferConsumerCopy;
    }


    Consumer(const Consumer& other) : 
    id(other.id), type(other.type), frequency(other.frequency), wrkld(other.wrkld), needMoreTasks(other.needMoreTasks), taskBufferConsumer(128), taskBufferConsumerCopy(other.taskBufferConsumerCopy) {
    for (Task* task : taskBufferConsumerCopy) {
            taskBufferConsumer.push(task);
        }
    }

    //não dá para aceder ao indice do taskBufferConsumer então criei uma cópia e copio tudo de uma vez



    Consumer(int id, ConsumerType type, double frequency) 
        : id(id), type(type), frequency(frequency), taskBufferConsumer(128) {} 



    bool pushTask(Task* task) {
        taskBufferConsumerCopy.push_back(task);
        return taskBufferConsumer.push(task);
    }


    bool isTaskBufferEmpty(){

        return taskBufferConsumer.empty();
    }


    Task* popTask() {
        Task* task = nullptr;
            if (taskBufferConsumer.pop(task)) {
                taskBufferConsumerCopy.erase(taskBufferConsumerCopy.begin());
                //std::cout << "Consumer " << id << " popped task! \n";
                return task;
            } else {
                return nullptr;
            }
    }

    // bool pushTask(Task* task) {
    // bool success = taskBufferConsumer.push(task);
    //     if (success) {
    //         taskBufferConsumerCopy.push_back(task);
    //     }
    //     return success;
    // }

    // bool isTaskBufferEmpty(){
    //     return taskBufferConsumer.empty();
    // }

    // bool popTask(Task* task) {
    //     Task** task = taskBufferConsumer.front();
    //     taskBufferConsumer.pop();
    //     taskBufferConsumerCopy.erase(taskBufferConsumerCopy.begin());
    // }



};

#endif

