#ifndef CONSUMER_H
#define CONSUMER_H

#include <iostream>
#include <vector>
#include <deque>
#include <random>
#include <fstream>
#include <cmath>


class Consumer {
public:
    std::vector<std::deque<double>> clock_freqs;

    Consumer(int numConsumers) {
        initializeFrequencies(numConsumers);
    }

    void saveFrequenciesToFile() {
        std::ofstream file("frequencies.txt");
        if (file.is_open()) {
            int id = 0;
            for (const auto& deque : frequencies) {
                
                for (const auto& frequency : deque) {
                    
                    file << id << ' ' << frequency << '\n';
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
        return std::abs(distribution(gen));
    }
};

#endif
