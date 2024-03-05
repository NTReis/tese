#ifndef TASK_H
#define TASK_H

#include <iostream>
#include <random>
#include <unistd.h>

class Task {
public:
    int id;
    bool regular;

    int getId() const {
        return id;
    }

    // Constructor
    Task(int task_id) : id(task_id) {
        setRandomRegular();
    }

    void run() {
        if (regular) {
            std::cout << "Regular task " << id << "\n";
            usleep(200);
        } else {
            std::cout << "Irregular task " << id << ": ";
            std::random_device rd;
            std::mt19937 gen(rd());
            std::normal_distribution<double> distribution(0.51, 0.5);

            double random_value = std::abs(distribution(gen));
            usleep(static_cast<int>(random_value * 200));

            std::cout << random_value << "\n";
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
