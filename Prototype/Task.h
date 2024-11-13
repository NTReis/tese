#ifndef TASK_H
#define TASK_H

#include <iostream>
#include <random>
#include <unistd.h>
#include <mutex>

enum TaskType {
    Regular,
    Irregular
};

class Task {
public:
    int id;
    int regular;
    float rank;
    int temp;
    int simulated_error;

    float computation_avg=10;

    std::vector<int> parents;


    float getTemp() const {
        return static_cast<float>(cpi * instructions);
    }

    float getPred() const {
        return static_cast<float>(simulated_error);
    }

    void setError(float error) {
        simulated_error = error;
    }

    
    float getRank() const {
        return rank;
    }

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
    if (regular == 0) {
        return TaskType::Regular;
    } else {
        return TaskType::Irregular;
    }
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
        //setRandomRegular();
    }



    float run(float frequency) {

        setRandomRegular();
        
        if (regular==0) {

            TaskType type = Regular;
            instructions = 200;
            cpi = 1.0;
            float duration = (instructions * cpi)/frequency;

            
            std::cout << "Regular task " << id << ": " << duration << "\n" << std::flush;
            
            usleep(duration);

            return(duration);

        } else {

            TaskType type = Irregular;
            std::cout << "Irregular task " << id << ": ";
            std::random_device rd;
            std::mt19937 gen(rd());
            std::normal_distribution<float> distribution(0.51, 0.5);

            float random_value = std::abs(distribution(gen));
            float cpi = std::abs(distribution(gen));

            instructions = random_value * 200;
            
            float duration = (instructions * cpi)/frequency;

            std::cout << duration << "\n" << std::flush;

            usleep(static_cast<int>(duration));

            return(duration);


        }
    }

    float runfromfile(float frequency) {
        
        if (regular == 0) {

            float duration = (instructions * cpi)/frequency;
      
            std::cout << "Regular task " << id << ": " << duration <<  "\n";
        
            usleep(duration);

            return duration;

        } else {

            float duration = (instructions * cpi)/frequency;
            
            std::cout << "Irregular task " << id << ": " << duration << "\n";
        
            usleep(duration);

            return duration;

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

