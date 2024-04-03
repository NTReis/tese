#ifndef TASK_H
#define TASK_H

#include <iostream>
#include <random>
#include <unistd.h>

enum TaskType {
    Regular,
    Irregular
};

class Task {
public:
    int id;
    int regular;
    int getId() const {
        return id;
    }
    double duration;
    double getDuration() const {
        return duration;
    }

    // Constructor
    Task(int task_id, TaskType type, int duration) : id(task_id), regular(type), duration(duration) {
        setRandomRegular();
    }

    TaskType getType() const {
        return regular ? Regular : Irregular;
    }


    void run(double frequency) {
        
        if (regular) {
            TaskType type = Regular;
            std::cout << "Regular task " << id << ": \n";
            duration = 200;
            usleep(duration*frequency);

        } else {
            TaskType type = Irregular;
            std::cout << "Irregular task " << id << ": \n";
            std::random_device rd;
            std::mt19937 gen(rd());
            std::normal_distribution<double> distribution(0.51, 0.5);

            double random_value = std::abs(distribution(gen));
            duration = random_value * 200;
            
            usleep(static_cast<int>(duration*frequency));


        }
    }

    void runfromfile(double frequency) {
        
        if (regular) {
            
            std::cout << "Regular task " << id << ": " << duration <<  "\n";
            usleep(duration*frequency);

        } else {

            std::cout << "Irregular task " << id << ": " << duration << "\n";
        
            usleep(duration*frequency);

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

#endif

