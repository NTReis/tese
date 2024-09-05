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

#include <vector>
#include <iostream>


std::vector<std::vector<int>> communication_cost_dag_test = {
    {0, 10, 12, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 13, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 14, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 15, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 15, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 17, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 18, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 19, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 11, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 21, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 31, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 41, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 51, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 61, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 71, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 81, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 19},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
};


std::vector<std::vector<float>> computationCosts; //tasks and their computation costs on each processor

std::vector<std::pair<int, float>> taskRanks; //tasks and their ranks

//std::vector<std::pair<int, bool>> visited; //tasks and their visited status


Scheduler(){
    }


float setCommunicationCost(int taskI, int taskJ) {
    // Random number generator
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(0, 1);

    return dist(gen);
}

void initializeData(int taskCount, std::vector<Consumer>& consumerList, boost::lockfree::queue<Task*, boost::lockfree::fixed_sized<false>>& taskBuffer) {
    std::cout << "Initializing data...\n" << std::endl;
    //int consumerCount = consumerList.size();
    communication_cost_dag.clear();

    for (int i = 0; i < taskCount; i++) {
        std::vector<int> v(taskCount);
        communication_cost_dag.push_back(v);
        
        for (int j = 0; j < taskCount; j++) {
            Task* taskPtr = nullptr;  // Initialize to nullptr
            
            // Pop a task from the buffer
            if (taskBuffer.pop(taskPtr) && taskPtr != nullptr) {
                int temp = taskPtr->getTemp();
                communication_cost_dag[i][j] = setCommunicationCost(i, j)*temp;

                //std::cout << "Task " << i << " to Task " << j << " has communication cost " << communication_cost_dag[i][j] << std::endl;
                
                //NAO CHEGO A UTILIZAR OCMO NOVO METODO Add the task to the parents vector if the communication cost is greater than zero
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


//experiemtnar usar o algoritmo de khan para calcular os ranks

std::vector<float> rankCalculate(int taskCount, boost::lockfree::queue<Task*, boost::lockfree::fixed_sized<false>>& taskBuffer) {
    std::vector<float> ranks(taskCount, 0.0f);
    std::vector<int> inDegree(taskCount, 0);
    std::vector<std::vector<int>> successors(taskCount);

    
    // std::cout << "communication_cost_dag matrix:" << std::endl;
    // for (int i = 0; i < taskCount; ++i) {
    //     for (int j = 0; j < taskCount; ++j) {
    //         std::cout << communication_cost_dag_test[i][j] << " ";
    //     }c
    //     std::cout << std::endl;
    // }

    // Calculate in-degrees and successors
    for (int i = 0; i < taskCount; ++i) {
        for (int j = 0; j < taskCount; ++j) {
            if (communication_cost_dag_test[i][j] > 0) {
                successors[i].push_back(j);
                ++inDegree[j];
                //std::cout << "Task " << i << " in-degree " << inDegree[j] << std::endl;
            }
        }
    }

    //para funcionar tem que haver pelo menos uma task com o in-degree a 0 por isso 
    
    
    std::queue<int> zeroInDegreeQueue;

    for (int i = 0; i < taskCount; ++i) {
        if (inDegree[i] == 0) {
            zeroInDegreeQueue.push(i);
            //std::cout << "Task " << i << " has in-degree 0." << std::endl;
         }//  else
        // { std::cout << "Task " << i << " has in-degree " << inDegree[i] << std::endl;}
        
    }

    
   //std::lock_guard<std::mutex> guard(mtx);
    while (!zeroInDegreeQueue.empty()) {

        int taskID = zeroInDegreeQueue.front();
        
        zeroInDegreeQueue.pop();

        //std::cout << zeroInDegreeQueue.size() << "SIZe" << std::endl;


        float max_rank = 0;
        int comp_avg = 0;

        // Calculate rank for the current node
        for (int succ : successors[taskID]) {
            max_rank = std::max(max_rank, ranks[succ] + communication_cost_dag_test[taskID][succ]);

            //std::cout << "Task " << taskID << " has edge to Task " << succ << std::endl;
        }

         
        //for (int j = 0; j < taskCount; j++){
            
            std::lock_guard<std::mutex> guard(mtx);
            Task* taskPtr = nullptr;

            

            
            if (taskBuffer.pop(taskPtr)) {

                    if (taskPtr != nullptr && taskPtr->id == taskID) {
                        comp_avg = taskPtr->computation_avg;

                        //std::cout << "Task " << taskID << " has computation average " << comp_avg << std::endl;
                        
                        taskPtr->rank = max_rank + comp_avg;
                        ranks[taskID] = taskPtr->rank;

                        //std::cout << "!!!!!!!!!!Task " << taskID << " has rank " << taskPtr->rank << std::endl;
                    }

                
                    taskBuffer.push(taskPtr);

                    
            } else {
                    std::cerr << "Error: Failed to pop task from buffer or taskPtr is nullptr." << std::endl;
                    break;
                }

            
        
        //}

        // Aqui temos que retirar indregrees de todos os sucessores e os que tiverem in-degree 0 tem que ser adicionados à queue 
        for (int succ : successors[taskID]) {
            if (--inDegree[succ] == 0) {
                zeroInDegreeQueue.push(succ);
            }
        }
    }
    

    return ranks;
}


void sortRank(int taskCount, boost::lockfree::queue<Task*, boost::lockfree::fixed_sized<false>>& taskBuffer) {
    
    std::vector rankings = rankCalculate(taskCount, taskBuffer);

    int minrank = 0;
    
    // Collect task ranks
    for (int i = 0; i < taskCount; ++i) {
        Task* taskPtr = nullptr;  // Initialize to nullptr
        
        
            std::lock_guard<std::mutex> guard(mtx); 
            
            if (taskBuffer.pop(taskPtr) && taskPtr != nullptr) {
                // Calculate rank for the task
                
                float rank = taskPtr->rank;

                //std::cout << "Task " << taskPtr->id << " has a rank of " << taskPtr->rank << std::endl;

                taskBufferCopy.push_back(taskPtr);
  

                taskBuffer.push(taskPtr);

            } else {
                std::cerr << "Error: Failed to pop task from buffer or taskPtr is nullptr." << std::endl;
            }
        
    }

    
    std::sort(taskBufferCopy.begin(), taskBufferCopy.end(), [](Task* a, Task* b) {
        return a->rank < b->rank;
    });


    // for (int i = 0; i < 20 ; i++){
    //     std::cout << "!!!!!!!!!Task " << taskBufferCopy[i]->id << " has rank " << taskBufferCopy[i]->rank << std::endl;
    // }


    for (int i = 0; i < taskCount; ++i) {
        Task* taskPtr = taskBufferCopy.front();
        taskBufferCopy.pop_front();
        updatedTaskBuffer.push(taskPtr);
    }



}


void distributeTasks(std::vector<Consumer>& consumerList, int chunkSize, boost::lockfree::spsc_queue<Task*>& taskBuffer) {
    Task* taskPtr = nullptr;

    while (true) {
        // Attempt to pop a task from the buffer
        bool popped = taskBuffer.pop(taskPtr);

        if (!popped) {
            // Buffer is empty; exit the loop
            std::cout << "\nNo more tasks in buffer. Exiting distribution loop.\n" << std::endl;
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
                float est = cons.getWrkld()*3000; // Tempo estimado que falta para terminar o trabalho ()
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
    std::cout << "\nSetting computation costs...\n" << std::endl;

    initializeData(taskCount, consumerList, taskBuffer);

    

    
    sortRank(taskCount, taskBuffer);
    std::cout << "Sorting ranks...\n" << std::endl;

  
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

// float rankCalculate(int nodeID, int taskCount, boost::lockfree::queue<Task*, boost::lockfree::fixed_sized<false>>& taskBuffer) {
    
//     if (visited[nodeID].second) {
//         std::cerr << "Cycle detected at node " << nodeID << ". Breaking recursion." << std::endl;
//         return 0;  // You can return a default value or handle it differently
//     }


//     visited[nodeID].second = true;
    
    
//     float max_rank = 0;
//     int comp_avg = 0;  

//     std::cout << "Calculating rank for node " << nodeID << std::endl;

//     //Recursive
//     for (int i = 0; i < taskCount; i++) {
//         if (i != nodeID && communication_cost_dag[nodeID][i] > 0) {
//             std::cout << "Node " << nodeID << " has edge to Node " << i << std::endl;

//             float succ_rank = rankCalculate(i, taskCount, taskBuffer); // Recursive call
//             max_rank = std::max(max_rank, succ_rank + communication_cost_dag[nodeID][i]);
//         }
//     }

//     {
//         std::lock_guard<std::mutex> guard(mtx);
//         Task* taskPtr = nullptr;  // Initialize to nullptr

//        
//         if (taskBuffer.pop(taskPtr) && taskPtr != nullptr) {
//             comp_avg = taskPtr->computation_avg; // Access computation average

//             
//             taskPtr->rank = max_rank + comp_avg;

//             std::cout << "Task " << taskPtr->getId() << " has rank " << taskPtr->getRank() << std::endl;
//             std::cout << "DEBUG" << std::endl;

//             taskBuffer.push(taskPtr);
//         } else {
//             
//             std::cerr << "Error: Failed to pop task from buffer or taskPtr is nullptr." << std::endl;
//         }
//     }

//     visited[nodeID].second = false;

//     return max_rank + comp_avg;
// }


};

#endif
