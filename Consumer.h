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

    ConsumerType type;

    std::vector<std::deque<double>> clock_freqs;

    Consumer(int numConsumers, ConsumerType type) : type(type) {
        initializeFrequencies(numConsumers);
    }

    void saveFrequenciesToFile() {
        std::ofstream file("frequencies.txt");
        if (file.is_open()) {
            int id = 0;
            for (const auto& deque : frequencies) {
                for (const auto& frequency : deque) {
                    file << id << ' ' << frequency << ' ';
                    if (type == ConsumerType::CPU) {
                        file << "CPU\n";
                    } else {
                        file << "GPU\n";
                    }
                    ++id;
                }
            }
        } else {
            std::cerr << "Error: Could not open frequencies.txt\n";
        }
        file.close();
    }

    void loadFreqs() {
        std::ifstream file("frequencies.txt");
        if (file.is_open()) {
            int id = 0;
            double frequency;
            while (file >> id >> frequency) {
                clock_freqs[id].push_back(frequency);
                ++id;
            }
        } else {
            std::cerr << "Error: Could not open frequencies.txt\n";
        }
        file.close();


    }

private:

    std::vector<std::deque<double>> frequencies;

    void initializeFrequencies(int numConsumers) {
        frequencies.resize(numConsumers);
        clock_freqs.resize(numConsumers);

        for (int i = 0; i < numConsumers; ++i) {
            frequencies[i].push_back(generateRandomFrequency());
        }
    }

    double generateRandomFrequency() {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::normal_distribution<double> distribution(0.1, 2);
        double ret = std::abs(distribution(gen));

        if (type == ConsumerType::CPU) {
            ret *= 1.2;
        } else {
            ret *= 0.8;
        }

        return ret;
    }
};

#endif

// #ifndef CONSUMER_H
// #define CONSUMER_H

// #include <iostream>
// #include <vector>
// #include <deque>
// #include <random>
// #include <fstream>
// #include <cmath>

// enum ConsumerType {
//     CPU,
//     GPU
// };

// class Consumer {
// public:
//     ConsumerType type;
//     std::vector<std::deque<double, ConsumerType>> clock_freqs;

//     Consumer(int numConsumers, int numWorkers, ConsumerType consumerType) : type(consumerType) {
//         initializeFrequencies(numConsumers, numWorkers);
//     }

//     void saveFrequenciesToFile() {
//         std::ofstream file("frequencies.txt");
//         if (file.is_open()) {
//             int id = 0;
//             for (const auto& deque : frequencies) {
//                 for (const auto& entry : deque) {
//                     double frequency;
//                     ConsumerType entryType;
//                     std::tie(frequency, entryType) = entry;

//                     file << id << ' ' << frequency << ' ';
//                     if (entryType == ConsumerType::CPU) {
//                         file << "CPU\n";
//                     } else {
//                         file << "GPU\n";
//                     }
//                     ++id;
//                 }
//             }
//         } else {
//             std::cerr << "Error: Could not open frequencies.txt\n";
//         }
//         file.close();
//     }


//     void loadFreqs() {
//         std::ifstream file("frequencies.txt");
//         if (file.is_open()) {
//             int id = 0;
//             double frequency;
//             ConsumerType Type;
//             while (file >> id >> frequency) {
//                 clock_freqs[id].emplace_back(frequency, type);
//             }
//         } else {
//             std::cerr << "Error: Could not open frequencies.txt\n";
//         }
//         file.close();
//     }

// private:

//     std::vector<std::deque<std::tuple<double, ConsumerType>>> frequencies;


//     void initializeFrequencies(int numConsumers, int numWorkers) {
//         clock_freqs.resize(numConsumers);
//         frequencies.resize(numConsumers);

//         for (int i = 0; i < numConsumers; ++i) {
//             if (i < numWorkers) {
//                 frequencies[i].emplace_back(generateRandomFrequency(), type);
//             } else {
//                 // If we've created the specified number of workers, create the remaining as the opposite type
//                 ConsumerType oppositeType = (type == ConsumerType::CPU) ? ConsumerType::GPU : ConsumerType::CPU;
//                 frequencies[i].emplace_back(generateRandomFrequency(oppositeType), type);
//             }
//         }
//     }

//     double generateRandomFrequency(ConsumerType targetType = ConsumerType::CPU) {
//         std::random_device rd;
//         std::mt19937 gen(rd());
//         std::normal_distribution<double> distribution(0.1, 2);
//         double ret = std::abs(distribution(gen));

//         if (targetType == ConsumerType::CPU) {
//             ret *= 1.2;
//         } else {
//             ret *= 0.8;
//         }

//         return ret;
//     }
// };

// #endif

