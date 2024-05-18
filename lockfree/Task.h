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
    double instructions;
    double getInstructions() const {
        return instructions;
    }

    double cpi;
    double getCPI() const {
        return cpi;
    }
    TaskType getType() const {
        return regular ? Regular : Irregular;
    }

  
    Task() = default;

    ~Task() = default;

    Task& operator=(const Task& other) = default;

    Task(const Task& other){
        id = other.getId(); 
        instructions = other.getInstructions();
        cpi = other.getCPI();
        regular = other.getType();

    }


    Task(int task_id, TaskType type, int instructions, double cpi) : id(task_id), regular(type), instructions(instructions), cpi(cpi) {
        setRandomRegular();
    }



    void run(double frequency) {
        
        if (regular) {
            TaskType type = Regular;
            instructions = 200;
            cpi = 1.0;
            double duration = instructions * cpi;
            std::cout << "Regular task " << id << ": " << duration << "\n";
            usleep(duration*frequency);

        } else {
            TaskType type = Irregular;
            std::cout << "Irregular task " << id << ": ";
            std::random_device rd;
            std::mt19937 gen(rd());
            std::normal_distribution<double> distribution(0.51, 0.5);

            double random_value = std::abs(distribution(gen));
            double cpi = std::abs(distribution(gen));

            instructions = random_value * 200;
            
            double duration = instructions * cpi;

            std::cout << duration << "\n";

            usleep(static_cast<int>(duration*frequency));


        }
    }

    void runfromfile(double frequency) {
        
        if (regular) {

            double duration = instructions * cpi;
            
            std::cout << "Regular task " << id << ": " << duration <<  "\n";
            usleep(duration/frequency);

        } else {

            double duration = instructions * cpi;

            std::cout << "Irregular task " << id << ": " << duration << "\n";
        
            usleep(duration/frequency);

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

