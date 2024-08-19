#ifndef HEFT_H
#define HEFT_H

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
#include <vector> 
#include <numeric> 
#include <algorithm> 

class Scheduler{

private:

std::mutex mtx; // Mutex to protect the print operation

bool isTaskBufferEmpty(boost::lockfree::queue<Task*, boost::lockfree::fixed_sized<false>> taskBuffer) {
    return taskBuffer.empty();

}



public:

boost::lockfree::spsc_queue<Task*> updatedTaskBuffer{128}; 

std::deque<Task*> taskBufferCopy;

bool schedulers_finished = false;
std:: vector<int> parents; // indexes of all the parents of a particular node

std::vector<std::vector<int>> communication_cost_dag; //tasks and their communication costs between each other

std::vector<std::vector<float>> computationCosts; //tasks and their computation costs on each processor

std::vector<std::pair<int, float>> taskRanks; //tasks and their ranks



Scheduler(){
    }


void initializeData(int taskCount, std::vector<Consumer>& consumerList, boost::lockfree::queue<Task*, boost::lockfree::fixed_sized<false>>& taskBuffer) {
    std::cout << "Initializing data..." << std::endl;
    int consumerCount = consumerList.size();

    for (int i = 0; i < taskCount; i++) {
        std::vector<int> v(consumerCount);
        communication_cost_dag.push_back(v);
        
        for (int j = 0; j < taskCount; j++) {
            Task* taskPtr = nullptr;  // Initialize to nullptr
            
            // Pop a task from the buffer
            if (taskBuffer.pop(taskPtr) && taskPtr != nullptr) {
                int temp = taskPtr->getTemp();
                communication_cost_dag[i][j] = temp;

                //std::cout << "Task " << i << " to Task " << j << " has communication cost " << communication_cost_dag[i][j] << std::endl;
                
                // Add the task to the parents vector if the communication cost is greater than zero
                if (communication_cost_dag[i][j] > 0) {
                    taskPtr->parents.push_back(i);
                }

                
                //std::cout << "Task " << i << " has " << taskPtr->parents.size() << " parents." << std::endl;

                
                taskBuffer.push(taskPtr);
            } else {
                // Handle the case where no task is available or taskPtr is nullptr
                std::cerr << "Error: Failed to pop task from buffer or taskPtr is nullptr." << std::endl;
            }
        }
    }
}

void setComputationCost(Consumer& cons, int taskCount, boost::lockfree::queue<Task*, boost::lockfree::fixed_sized<false>>& taskBuffer, int consumerCount) {



    computationCosts.resize(taskCount, std::vector<float>(consumerCount, 0.0f));
    

    for (int i = 0; i < taskCount; i++) {
        std::lock_guard<std::mutex> guard(mtx);

        Task* taskPtr = nullptr;
        if (taskBuffer.pop(taskPtr)) {
            if (taskPtr != nullptr) {
                int j = cons.getId();

                // Ensure that the consumer ID is valid
                if (j >= 0 && j < consumerCount) {
                    // Ensure that frequency and taskPtr->temp are valid
                    float cost = taskPtr->getTemp() * cons.frequency;
                    computationCosts[i][j] = cost;

                    // Debug output
                    //std::cout << "Task " << i << " on processor " << j << " has computation cost " << computationCosts[i][j] << std::endl;
                } else {
                    std::cerr << "Consumer ID out of bounds: " << j << std::endl;
                }
                taskBuffer.push(taskPtr);
            } else {
                std::cerr << "Popped task pointer is null." << std::endl;
            }
        } else {
            std::cerr << "Failed to pop task from buffer or task buffer is empty." << std::endl;
        }
    }
}


float getComputationCost(int taskID, int consumerID) {
    return computationCosts[taskID][consumerID];
}

// float rankCalculate(int node, int taskCount, boost::lockfree::queue<Task*, boost::lockfree::fixed_sized<false>>& taskBuffer) {
//     float max_rank = 0;
//     int comp_avg = 0;  // Ensure proper initialization

//     std::cout << "Calculating rank for node " << node << std::endl;

//     // Recursive calculation for all successor tasks
//     for (int i = 0; i < taskCount; i++) {
//         if (communication_cost_dag[node][i] > 0) {
//             std::cout << "Node " << node << " has edge to Node " << i << std::endl;

//             float succ_rank = rankCalculate(i, taskCount, taskBuffer); // Recursive call
//             max_rank = std::max(max_rank, succ_rank + communication_cost_dag[node][i]);
//         }
//     }

//     {
//         std::lock_guard<std::mutex> guard(mtx);
//         Task* taskPtr = nullptr;  // Initialize to nullptr

//         // Pop a task from the buffer
//         if (taskBuffer.pop(taskPtr) && taskPtr != nullptr) {
//             comp_avg = taskPtr->computation_avg; // Access computation average

//             // Set rank and push the task back to the buffer
//             taskPtr->rank = max_rank + comp_avg;

//             std::cout << "Task " << taskPtr->getId() << " has rank " << taskPtr->getRank() << std::endl;
//             std::cout << "DEBUG" << std::endl;

//             taskBuffer.push(taskPtr);
//         } else {
//             // Handle the case where no task is available or taskPtr is nullptr
//             std::cerr << "Error: Failed to pop task from buffer or taskPtr is nullptr." << std::endl;
//         }
//     }

//     return max_rank + comp_avg;
// }

float rankCalculate(int node, int taskCount, boost::lockfree::queue<Task*, boost::lockfree::fixed_sized<false>>& taskBuffer){
    
    float rank = node;

    
    //std::cout << "Task " << node << " has rank " << rank << std::endl;
    
    return rank;
}

void sortRank(int taskCount, boost::lockfree::queue<Task*, boost::lockfree::fixed_sized<false>>& taskBuffer) {
    // This vector holds pairs of task IDs and their ranks
    std::vector<std::pair<int, float>> taskRanks;

    int minrank = 0;
    
    // Collect task ranks
    for (int i = 0; i < taskCount; ++i) {
        Task* taskPtr = nullptr;  // Initialize to nullptr
        
        
            std::lock_guard<std::mutex> guard(mtx); // Lock for thread safety
            
            if (taskBuffer.pop(taskPtr) && taskPtr != nullptr) {
                // Calculate rank for the task
                float rank = rankCalculate(taskPtr->id, taskCount, taskBuffer);

                if (rank < minrank){
                    minrank = rank;
                    taskBufferCopy.push_back(taskPtr);
                } else {
                    taskBufferCopy.push_front(taskPtr);
                }   

                taskBuffer.push(taskPtr);

            } else {
                std::cerr << "Error: Failed to pop task from buffer or taskPtr is nullptr." << std::endl;
            }
        
    }

    for (int i = 0; i < taskCount; ++i) {
        Task* taskPtr = taskBufferCopy.front();
        taskBufferCopy.pop_front();
        updatedTaskBuffer.push(taskPtr);
    }

    std::cout << taskRanks.size() << " task ranks collected." << std::endl;

    // Sort based on rank in descending order
    std::sort(taskRanks.begin(), taskRanks.end(), [](const auto& a, const auto& b) {
        return a.second > b.second;
    });


    
}


void distributeTasks(std::vector<Consumer>& consumerList, int chunkSize, boost::lockfree::spsc_queue<Task*>& taskBuffer) {
    Task* taskPtr = nullptr;

    while (true) {
        // Attempt to pop a task from the buffer
        bool popped = taskBuffer.pop(taskPtr);

        if (!popped) {
            // Buffer is empty; exit the loop
            std::cout << "No more tasks in buffer. Exiting distribution loop." << std::endl;
            break;
        }

        // Check if the task pointer is valid
        if (taskPtr == nullptr) {
            std::cerr << "Error: Popped task is nullptr." << std::endl;
            continue; // Skip to the next iteration
        }

        //std::cout << "Popped task " << taskPtr->getId() << " from buffer." << std::endl;

        Consumer* bestConsumer = nullptr;
        float minEFT = std::numeric_limits<float>::max();

        
        std::lock_guard<std::mutex> guard(mtx);

        // Evaluate each consumer to find the best one
        for (Consumer& cons : consumerList) {
            if (!cons.getNeedMoreTasks()) {
                float est = cons.getWrkld()*3000; // Tempo estimado que falta para terminar o trabalho
                float eft = est + getComputationCost(taskPtr->getId(), cons.getId());

                if (eft < minEFT) {
                    minEFT = eft;
                    bestConsumer = &cons;
                }
            }
        }

        if (bestConsumer) {
            // Distribute the task to the best consumer
            bestConsumer->pushTask(taskPtr);
            std::cout << "Scheduler distributing task " << taskPtr->getId() << " to Consumer " << bestConsumer->getId() << std::endl;
            bestConsumer->wrkld += 1;
        } else {
            // No suitable consumer found; push task back to buffer
            std::cerr << "No suitable consumer found for task " << taskPtr->getId() << ". Pushing task back to buffer." << std::endl;
            if (!taskBuffer.push(taskPtr)) {
                std::cerr << "Error: Failed to push task back to buffer." << std::endl;
            }
        }
    }
}





// void startScheduling(int totalTasks, std::vector<Consumer>& consumerList, boost::lockfree::queue<Task*, boost::lockfree::fixed_sized<false>>& taskBuffer) {
    
    
//     int totalConsumers = consumerList.size();

//     int chunkSize = totalTasks/totalConsumers;
//     int resto = totalTasks%totalConsumers;

//     for (int i = 0; i < totalConsumers; ++i) {
//         Consumer& cons = consumerList[i];

//         if ((i+1)!=totalConsumers){
//             distributeTasks(cons, chunkSize, taskBuffer);
//             totalTasks -= chunkSize;

//         } else {
//             int restante = chunkSize+resto;
//             distributeTasks(cons, restante, taskBuffer);
//             totalTasks = 0;
//         }
//     } 

// // Work-stealing phase
//     bool allConsumersFinished = false;
//     while (!allConsumersFinished) {
//         allConsumersFinished = true;        

//         for (int i = 0; i < totalConsumers; ++i) {
//             Consumer& cons = consumerList[i];

//             if (cons.getNeedMoreTasks()) {
//                 redistributeTasks(consumerList);
//                 cons.setNeedMoreTasks(false); // Reset the flag
//             }


//             // Check if this consumer has finished its tasks
//             if (!cons.getWrkld()==0) {
//                 allConsumersFinished = false;
//             }
//         }
        
//         if (allConsumersFinished) {
//             std::cout << "All consumers have finished their tasks. No stealing required." << std::endl;
//             break;
//         }
//     }
// }


void startScheduling(int taskCount, std::vector<Consumer>& consumerList, boost::lockfree::queue<Task*, boost::lockfree::fixed_sized<false>>& taskBuffer) {
    int totalConsumers=consumerList.size();

    for (int i = 0; i < totalConsumers; ++i) {
        Consumer& cons = consumerList[i];
        setComputationCost(cons, taskCount, taskBuffer, totalConsumers);
    }
    std::cout << "Setting computation costs..." << std::endl;

    initializeData(taskCount, consumerList, taskBuffer);

    

    
    sortRank(taskCount, taskBuffer);
    std::cout << "Sorting ranks..." << std::endl;

  
    distributeTasks(consumerList, taskCount, updatedTaskBuffer);


}


void setSchedulersFinished(bool value){
    std::lock_guard<std::mutex> guard(mtx);

    schedulers_finished = value;
    
}

bool schedulerFinished(){
    std::lock_guard<std::mutex> guard(mtx);

    bool value = schedulers_finished;

    return value;

}

};

#endif
