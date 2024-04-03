#ifndef CONSUMER_H
#define CONSUMER_H

#include <iostream>
#include <vector>
#include <deque>
#include <random>
#include <fstream>
#include <cmath>

enum ConsumerType {
    CPU,
    GPU
};

class Consumer {
public:

    std::deque<Task> taskBufferConsumer; 

    ConsumerType type;
    int id;
    double frequency;
    int wrkld;
    //int type;

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


    Consumer(int id, ConsumerType type, double frequency) : id(id) , type(type) ,frequency(frequency) {
        
    }

};


#endif

