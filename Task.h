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

    // Constructor
    Task(int task_id) : id(task_id) {
        setRandomRegular();
    }

    TaskType getType() const {
        return regular ? Regular : Irregular;
    }

    void run(double frequency) {
        
        if (regular) {
            TaskType type = Regular;
            std::cout << "Regular task " << id << "\n";
            usleep(200*frequency);
        } else {
            TaskType type = Irregular;
            std::cout << "Irregular task " << id << ": ";
            std::random_device rd;
            std::mt19937 gen(rd());
            std::normal_distribution<double> distribution(0.51, 0.5);

            double random_value = std::abs(distribution(gen));
            usleep(static_cast<int>(random_value * 200)*frequency);

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

// #ifndef TASK_H
// #define TASK_H

// #include <iostream>
// #include <random>
// #include <unistd.h>

// enum TaskType {
//     Regular,
//     Irregular
// };

// class Task {
// public:
//     int id;

//     // Constructor with an optional parameter to set the task type
//     Task(int task_id, TaskType type = Regular) : id(task_id), type(type) {
//         if (type == Regular) {
//             setRegular();
//         } else {
//             setIrregular();
//         }
//     }

//     TaskType getType() const {
//         return type;
//     }

//     int getId() const {
//         return id;
//     }

//     void run(double frequency) {
//         std::cout << "Task " << id << " (" << (type == Regular ? "Regular" : "Irregular") << "): ";

//         if (type == Irregular) {
//             std::random_device rd;
//             std::mt19937 gen(rd());
//             std::normal_distribution<double> distribution(0.51, 0.5);

//             double random_value = std::abs(distribution(gen));
//             usleep(static_cast<int>(random_value * 200) * frequency);

//             std::cout << random_value << "\n";
//         } else {
//             std::cout << "Executing regular task\n";
//             usleep(200 * frequency);
//         }
//     }

// private:
//     TaskType type;

//     void setRegular() {
//         type = Regular;
//     }

//     void setIrregular() {
//         type = Irregular;
//     }
// };

// #endif
