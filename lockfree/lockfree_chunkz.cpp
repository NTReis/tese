// g++ -g LockFree.cpp -o engine -lpthread -I/home/hondacivic/Boost/boost_1_82_0 -DD_LOL
//
//testar com isto 
//
// g++ -g LockFree.cpp -O2 -o engine -lpthread -I/home/hondacivic/Boost/boost_1_82_0 -DD_LOL
// g++ -g LockFree.cpp -O3 -o engine -lpthread -I/home/hondacivic/Boost/boost_1_82_0 -DD_LOL
//
//vtune
//source /opt/intel/oneapi/setvars.sh
//

#include <cmath>
#include <filesystem>
#include <regex>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <unistd.h>
#include <random>
#include <atomic>
#include "boost/lockfree/queue.hpp"
#include "Task.h"
#include "Consumer.h"
#include "Producer.h"
#include <boost/lockfree/spsc_queue.hpp>
#include <chrono>
//#include "Scheduler.h"
//#include "SchedulerStreaming.h"
//#include "SchedulerPredict.h"
//#include "SchedulerWS.h"
#include "Heft.h"



boost::lockfree::queue<Task*, boost::lockfree::fixed_sized<false>> taskBuffer(400000);

std::mutex idMutex; // Mutex to protect the increment operation for globalTaskId
std::mutex printMutex; // Mutex to protect the print operation
std::mutex fileMutex; // Mutex to protect the file operation
std::mutex mtx; // Mutex to protect the consumer workload
std::mutex mtx2; 
std::mutex consList; // Mutex to protect the consumer list
std::vector<Consumer> consumerlist;
std::vector<std::string> log_precursor;
std::condition_variable producer_cv;  // new condition variable for the scheduler
std::condition_variable tasksLoaded_cv;  // new condition variable for the scheduler
std::atomic<bool> producersFinished=false;
std::atomic<bool> tasksLoaded=false;
std::atomic<bool> schedulersFinished=false;
std::atomic<bool> useProducer=false;
std::atomic<int> taskCount=0;
std::atomic<int> consCount=0;
std::atomic<int> activeProducers(0);
std::atomic<int> globalTaskId(0); // Global variable to store the task ID
int log_counter = 0;
float flag;
bool requiresChunkSize = false;
int chunkSize = 5000;
Scheduler test;

std::string logname;
std::atomic<std::chrono::high_resolution_clock::time_point> globalStartTime;
std::atomic<std::chrono::high_resolution_clock::time_point> globalEndTime;
float totalExecutionTime;


void printTaskBuffer() {
    std::lock_guard<std::mutex> lock(printMutex);
    std::cout << "\n=== Task Buffer Contents ===\n";
    int count = 0;
    
    // Create temporary buffer to store tasks while printing
    std::vector<Task*> tempBuffer;
    
    // Pop and store all tasks
    Task* task;
    while (taskBuffer.pop(task)) {
        tempBuffer.push_back(task);
        std::cout << "Task " << task->getId() 
                  << " | Type: " << (task->getType() == TaskType::Regular ? "Regular" : "Irregular")
                  << " | Instructions: " << task->getInstructions()
                  << " | CPI: " << task->getCPI() << "\n";
        count++;
    }
    
    // Push all tasks back
    for (Task* t : tempBuffer) {
        while (!taskBuffer.push(t)) {
            std::this_thread::yield();
        }
    }
    
    std::cout << "Total tasks in buffer: " << count << "\n"
              << "========================\n\n";
}


// void savetaskFile(int id, TaskType type, float instructions, float cpi) {
//     //std::lock_guard<std::mutex> lock(fileMutex);
//
//     std::ofstream file("tasksProduced.txt", std::ios::app);
//     if (!file.is_open()) {
//         std::cerr << "Failed to open tasks.txt" << std::endl;
//         return;
//     }
//
//     file << id << ' ' << static_cast<int>(type) << ' ' << instructions << ' ' << cpi << '\n';
//     file.close();
// }

void loadtaskFile(const std::string& pathTaskFile) {
    std::ifstream file(pathTaskFile);
    if (file.is_open()) {
        int id;
        int type;
        float instructions;
        float cpi;
        while (file >> id >> type >> instructions >> cpi) {
            TaskType taskType = type == 0 ? TaskType::Regular : TaskType::Irregular;

            taskBuffer.push(new Task(id, taskType, instructions, cpi));
            taskCount++;

            //std::cout << "Task " << id << " " << taskType << " loaded\n";

        }
    } else {
        std::cerr << "Error: Could not open tasks.txt\n";
    }
    file.close();
}

void saveworkersFile(int consumers, int cpu) {
    std::ofstream file("workers.txt");

    if (file.is_open()) {
        for (int id = 0; id < consumers; ++id) {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::normal_distribution<float> distribution(0.1, 2);
            float ret = std::abs(distribution(gen));
            ConsumerType type;

            if (id < cpu) {
                type = ConsumerType::CPU;
            } else {
                type = ConsumerType::GPU;
            }

            float frequency = ret;

            if (type == ConsumerType::CPU) {
                ret *= 1.2;
            } else {
                ret *= 0.8;
            }

            Consumer cons(id, type, frequency);

            file << id << ' ' << static_cast<int>(cons.getType()) << ' ' << cons.getFreq() << '\n';
        }
    } else {
        std::cerr << "Error: Could not open workers.txt\n";
    }
    file.close();
}

void loadworkersFile(const std::string& pathWorkerFile) {
    std::ifstream file(pathWorkerFile);
    if (file.is_open()) {
        int id=0;
        int type=0; // Change the type to an integer.
        float frequency=0;
        while (file >> id >> type >> frequency) {

                consumerlist.push_back(Consumer(id, static_cast<ConsumerType>(type), frequency));
                consCount++;
            

            //std::cout << "Consumer " << consCount << " loaded\n";
        }
    } else {
        std::cerr << "Error: Could not open workers.txt\n";
    }
    file.close();

    //tasksLoaded_cv.notify_one();
}


void producer(int id, int elem) {

    int delay = 1;

    Producer producer(id,delay,elem);

    // Start producing tasks
    for (int i = 0; i < elem; ++i) {
        producer.produceSingleTask();
    }

    //DEBUG
    //std::cout << "ACTIVE PRODUCERS: " << activeProducers << "\n";

    // Decrement active producers and notify if last one
    if (--activeProducers == 0) {
        producer_cv.notify_all();
    }
   
    //printTaskBuffer();
}


void setup_producers(int num_producers, int total_elements) {
    activeProducers = num_producers;
    int producer_workload = total_elements / num_producers;
    int producer_remainder = total_elements % num_producers;
    
    std::vector<std::thread> producerThreads;
    producerThreads.reserve(num_producers);
    
    // Create and start producer threads
    for (int i = 0; i < num_producers; ++i) {
        int workload = producer_workload + (i < producer_remainder ? 1 : 0);
        producerThreads.push_back(std::thread(producer, i, workload));  // Use push_back instead
    }

    
    
    // Join all producer threads
    for (auto& thread : producerThreads) {
        thread.join();
    }

}


void simulateError (bool todo, int error, boost::lockfree::queue<Task*, boost::lockfree::fixed_sized<false>>& taskBuffer) {
    if (!todo) {
            for (int j = 0; j < taskCount; ++j) {
            Task* taskPtr;
            if (taskBuffer.pop(taskPtr)) {

                taskPtr->setError(taskPtr->getTemp());
                taskBuffer.push(taskPtr);
                


            }               
        }
    }
    else {
            for (int j = 0; j < taskCount; ++j) {
                Task* taskPtr;
                if (taskBuffer.pop(taskPtr)) {
                    float center = taskPtr->getTemp();

                    float range = center * (error / 100.0);
                    float stddev = range / 3.0; // 99.7% of values fall within ±3 standard deviations

                    std::random_device rd;
                    std::mt19937 gen(rd());
                    std::normal_distribution<float> distribution(center, range);

                    float pred;
            
                    pred = std::abs(distribution(gen));
                    taskPtr->setError(pred);    
                    //std::cout << "Task " << taskPtr->getId() << " error: " << pred << "| Real time:"<< taskPtr->getTemp() <<"\n";
                    taskBuffer.push(taskPtr);  
                }   
               
            }
            std::cout << "Finished setting error for " << taskCount << " tasks.\n";

        }
}

void scheduler(){

    //chunkSize = 60;

    flag =  chunkSize * 0.1;


    if (useProducer) {


        std::unique_lock<std::mutex> lk(mtx2);
        producer_cv.wait(lk, []{ 
            return activeProducers == 0 && taskCount > 0; 
        });
        
        
        std::cout << "Scheduler activated after producers finished." << std::endl;
    }

    std::cout << "Scheduler starting with chunksize: " << chunkSize << "\n" << std::endl;
    
    // std::thread debugThread([&]() {
    //     while(!schedulersFinished) {
    //         std::this_thread::sleep_for(std::chrono::seconds(2));
    //         size_t remainingTasks = 0;
    //         Task* taskPtr;
    //         std::vector<Task*> temp;
    //         while(taskBuffer.pop(taskPtr)) {
    //             temp.push_back(taskPtr);
    //             remainingTasks++;
    //         }
    //         for(Task* t : temp) taskBuffer.push(t);
            
    //         std::cout << "DEBUG - Tasks in main buffer: " << remainingTasks << std::endl;
    //         std::cout << "DEBUG - Consumer status:" << std::endl;
    //         for(int i=0; i < std::min(5, (int)consumerlist.size()); i++) {
    //             std::cout << "  Consumer " << i << ": tasks=" << consumerlist[i].getTaskCount() 
    //                     << " needsMore=" << consumerlist[i].getNeedMoreTasks() << std::endl;
    //         }
    //     }
    // });
    // debugThread.detach();


    // for(int i = 0; i<10; i++){
    //     Task* taskPtr;
    //     if (taskBuffer.pop(taskPtr)) {
        
    //     std::cout << taskPtr->getId() << " " << taskPtr->getType() << " " << taskPtr->getTemp() << " " << taskPtr->getCPI() << " " << std::endl;
    // };

       
    test.startScheduling(taskCount, chunkSize, consumerlist, taskBuffer);

    //test.startStreamingScheduling(activeProducers ,taskCount, chunksize, consumerlist, taskBuffer);
    test.setSchedulersFinished(true);
        

}

void scheduler_streaming() {
    //int chunksize = 500;
    flag = std::min(500.0, chunkSize * 0.1);
    
    std::cout << "Streaming scheduler starting with chunksize: " << chunkSize << "\n" << std::endl;
    
    // Keep scheduling until all producers are done AND task buffer is empty
    while (true) {
        // Check if we're done (producers finished AND no more tasks)
        if (activeProducers == 0 && taskBuffer.empty()) {
            // Double check after a small sleep to avoid race conditions
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            if (activeProducers == 0 && taskBuffer.empty()) {
                break;  // Exit the loop if condition is still true after waiting
            }
        }

        // Count tasks 
        size_t tasksAvailable = 0;
        std::vector<Task*> tempTasks;
        Task* taskPtr;
        
        
        while (taskBuffer.pop(taskPtr)) {
            tasksAvailable++;
            tempTasks.push_back(taskPtr);
        }
        
        
        for (Task* task : tempTasks) {
            taskBuffer.push(task);
        }

        // Schedule any available tasks if buffer has enough tasks or producers are done
        if (requiresChunkSize){
            if (tasksAvailable > 0 && (tasksAvailable >= chunkSize || activeProducers == 0)) {
                //test.startScheduling(taskCount, consumerlist, taskBuffer);

                test.startScheduling(taskCount, chunkSize, consumerlist, taskBuffer);
            }
        } else {
            if (tasksAvailable > 0 && activeProducers == 0) {
                //test.startScheduling(taskCount, consumerlist, taskBuffer);

                test.startScheduling(taskCount, chunkSize, consumerlist, taskBuffer);
            }
        }
        
        // Short sleep
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    std::cout << "Streaming scheduler finished." << std::endl;
    test.setSchedulersFinished(true);
}

void consumer (int id){
    std::lock_guard<std::mutex> lock(consList);

    if (id < 0 || id >= consumerlist.size()) {
    std::cerr << "Error: Invalid consumer id " << id << std::endl;
    return;
    }

    Consumer& cons = consumerlist[id];

    int i = taskCount;
    int workload = cons.getWrkld(); 

    // std::cout << "THIS IS THE WORKLOAD: " << workload << "\n";

    auto start = std::chrono::high_resolution_clock::now();

    int tasksProcessed = 0;

    while(!test.schedulerFinished() || !cons.isTaskBufferEmpty()) {
            
        
        if (!cons.isTaskBufferEmpty()){
            //new
            if (cons.isTaskBufferEmpty()) {
                continue;
            }
            //till here
        

            Task* taskPtr = cons.popTask();

            if (taskPtr == nullptr) {
                std::cerr << "Warning: Null task pointer from non-empty buffer for consumer " << id << std::endl;
                continue;
            }

            tasksProcessed++;

            if (tasksProcessed % 50 == 0) {  // Print every 50 tasks
                std::cout << "Consumer " << id << " has processed " 
                         << tasksProcessed << " tasks. Workload remaining: "
                         << cons.wrkld << "\n";
            }
                
            std::lock_guard<std::mutex> printlock(printMutex); 

            

            #ifdef D_LOL

            std::string consumer_type;

            if (cons.getType() == 0){consumer_type = "CPU";} else {consumer_type="GPU";}
            
            std::string temp = "Consumer " + std::to_string(cons.getId()) + "  |       " + consumer_type + "      |    ";
            log_precursor.push_back(temp);
            ++log_counter;

            #endif

            std::cout << "Consumer " << cons.getId() << ": " ;

            float clk_frq = cons.getFreq();

            if (taskPtr != nullptr){

                //std::this_thread::sleep_for(std::chrono::milliseconds(25)); 

                {
                    float duration = taskPtr->runfromfile(clk_frq);
                    
                    //ver o tempo desde que começou a consumir
                    auto now = std::chrono::high_resolution_clock::now();
                    std::chrono::duration<float> elapsed = now - start;
                    //std::cout << "Elapsed time: " << elapsed.count() << " seconds\n";
                    
          
                    #ifdef D_LOL

                    std::string task_type;

                    if (taskPtr->getType() == 0){task_type = " Regular ";} else {task_type="Irregular";}

                    std::string temp2 = std::to_string(taskPtr->getId()) + "    |  " + task_type + "  | " + std::to_string(duration) + " | " + std::to_string(elapsed.count()) + " seconds\n";
                    log_precursor.push_back(temp2);
                    ++log_counter;
                    //std::this_thread::sleep_for(std::chrono::milliseconds(10));

                    
                    #endif 


                   
                }  

                delete taskPtr;
                taskPtr = nullptr;

                //std::this_thread::sleep_for(std::chrono::milliseconds(100));


                

            } else 
                {
                    std::cerr << "Error: Null task pointer for consumer " << id << std::endl;
                    continue;
                    
                } 
                        
            std::lock_guard<std::mutex> guard(mtx);
            //mtx.lock();
            if (cons.wrkld > 0) {
                --cons.wrkld;

                if (cons.wrkld < flag && cons.wrkld > 0) {
                    cons.setNeedMoreTasks(true);

                    //std::cout << "FLAG!!" << cons.wrkld << "\n";
                    
                 }
                
            }


        }
        // After processing all tasks, if the consumer is out of work, request more
        if (cons.isTaskBufferEmpty() && cons.wrkld == 0 && !test.schedulerFinished()) {
            cons.setNeedMoreTasks(true);
        }
        
    } 

    #ifdef D_LOL

    std::filesystem::create_directories("logs");
    std::string logpath = "logs/" + logname;
    std::ofstream log_file(logpath);

    int l = 0;

    if (log_file.is_open()){

        for(int l; l < log_precursor.size(); l++){
            log_file << log_precursor[l];
        }

    } else {"Error: Could not access log file.\n";}

    log_file.close();

    #endif

        // std::cout << "!!!CONSUMER " << id << " FINISHED. Total tasks processed: " 
        //       << tasksProcessed << "\n";

}

void usage() {
    std::cout << "Usage: program_name "
              << "[-n num_elems num_consumers num_cpu num_producers [stream] | "
              << "-f path_worker_file path_tasks_file  | "
              << "-cc num_consumers num_cpu path_tasks_file  | "
              << "-cp num_elems path_worker_file num_producers [stream] | "
              << "-pred path_worker_file path_tasks_file error%]\n";
}

struct LogEntry {
    int consumerId;
    std::string consumerType;
    int taskId;
    std::string taskType;
    float duration;
    float elapsedTime;
};

std::vector<LogEntry> readLogFile (const std::string& pathLogFile){


    std::ifstream logFile("logs/" + pathLogFile);
    std::vector<LogEntry> logEntries;
    std::string line;

    //DEGUB
    std::ofstream tasksIDFile("tasksIDConsumed.csv");

    if (logFile.is_open()) {
        // Skip the header lines
        std::getline(logFile, line);
        std::getline(logFile, line);

    std::regex logEntryPattern(R"(Consumer\s(\d+)\s*\|\s*([a-zA-Z]+)\s*\|\s*(\d+)\s*\|\s*([a-zA-Z]+)\s*\|\s*(\d+\.\d+)\s*\|\s*(\d+\.\d+)\sseconds)");

    while (std::getline(logFile, line)) {
             std::smatch match;
        if (std::regex_search(line, match, logEntryPattern)) {
            LogEntry entry;
            entry.consumerId = std::stoi(match[1].str());
            entry.consumerType = match[2].str();
            entry.taskId = std::stoi(match[3].str());
            entry.taskType = match[4].str();
            entry.duration = std::stof(match[5].str());
            entry.elapsedTime = std::stof(match[6].str());

            logEntries.push_back(entry);

            tasksIDFile << entry.taskId << std::endl; //DEGUB

            // std::cout << "Parsed Log Entry: "
            //       << "Consumer ID: " << entry.consumerId
            //       << ", Consumer Type: " << entry.consumerType
            //       << ", Task ID: " << entry.taskId
            //       << ", Task Type: " << entry.taskType
            //       << ", Duration: " << entry.duration
            //       << ", Elapsed Time: " << entry.elapsedTime << " seconds" << std::endl;
        }
        }
        logFile.close();
        tasksIDFile.close();

        std::cout << "Total log entries read: " << logEntries.size() << std::endl;
    }

    return logEntries;
}


void execOverview(const std::vector<LogEntry>& logEntries, const std::string& pathLogFile) {
    if (logEntries.empty()) {
        std::cerr << "Log file is empty or could not be read." << std::endl;
        return;
    }

    // Find the first consumer's elapsed time
    int firstConsumerId = logEntries.front().consumerId;
    float fstConsumerET = 0.0;
    float lstConsumerET = 0.0;
    int i;

    for( i=0; i<logEntries.size(); i++){
        if (firstConsumerId == logEntries[i].consumerId) {
            fstConsumerET = logEntries[i].elapsedTime;
        }
        
    }

     for(int  j=0; j<logEntries.size(); j++){
        if (firstConsumerId != logEntries[j].consumerId) {
            lstConsumerET = logEntries[j].elapsedTime;
        }
        
    }

    

    float difference = lstConsumerET - fstConsumerET;

    int percentage = (std::abs(difference)*100)/lstConsumerET;


    //std::cout << "First consumer Ellapsed Time: " << fstConsumerET << " s" << std::endl;
    //std::cout << "Last consumer Ellapsed Time: " << lstConsumerET << " s" << std::endl;

    // Append the result to the log file
    std::ofstream logFile("logs/" + pathLogFile, std::ios_base::app);
    if (logFile.is_open()) {
        logFile << "\n Difference between first and last consumer: " << std::abs(difference) << "s" << " (" << percentage << "%" << " of overall Texec)" << std::endl;
        logFile.close();
    } else {
        std::cerr << "Unable to open log file for writing." << std::endl;
    }
}

int main(int argc, char* argv[]) {
    
    if (argc < 2) {
        std::cerr << "Error: No command provided.\n";
        usage();
        return 1;
    }

    std::string command = argv[1];
    if (command == "-n" && (argc == 6 || argc == 7)) {
        int elem = std::stoi(argv[2]);
        int consumers = std::stoi(argv[3]);
        int cpu = std::stoi(argv[4]);
        int producers = std::stoi(argv[5]);
        bool streamingMode = (argc == 7 && std::string(argv[6]) == "stream");


        #ifdef D_LOL

        //std::string logname;
        std::cout << "What is the log file name?: ";
        std::cin >> logname;

        if(logname.empty()) {
            logname = "log.csv"; // Default name if empty
        }
        
        if(logname.find(".csv") == std::string::npos) {
            logname += ".csv"; // Add .csv extension if not present
        }

        #endif


        std::cout << "Does your scheduler require chunksize? (y/n): ";
        char response;
        std::cin >> response;

        if (response == 'y' || response == 'Y') {
            requiresChunkSize = true;
            std::cout << "chunksize number?: ";
            std::cin >> chunkSize;
            
        // Validate input
        while (std::cin.fail() || chunkSize <= 0) {
            std::cin.clear(); // Clear error flags
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // Skip bad input
            std::cout << "Please enter a valid positive number: ";
            std::cin >> chunkSize;
        }
    
        std::cout << "Using chunk size: " << chunkSize << std::endl;
            } else {
            std::cout << "continuing without chunksize..." << std::endl;
            requiresChunkSize = false;
            }   



        if (streamingMode) {
            std::cout << "Using streaming mode: producers and scheduler run in parallel\n";
        }

        if (cpu>consumers) {
            std::cerr << "Error: The number of consumers is less than the number of CPU consumers. The number of cpu's will be reduced from " << cpu << " to " << consumers << ".\n" << std::endl;
            cpu=consumers; 
        }

        saveworkersFile(consumers, cpu);
        loadworkersFile("workers.txt");
        useProducer=true;

        std::ofstream clearFile("tasksProduced.txt", std::ios::trunc);
        clearFile.close();

        #ifdef D_LOL
    
        log_precursor.push_back("Consumer ID |  Consumer Type | Task ID |  Task Type  |   Duration  |  Ellapsed Time\n");
        ++log_counter;
        log_precursor.push_back( " -------------------------------------------------------------------------\n");   
        
        #endif

        int consumer_workload = (elem / consumers); 
        int producer_workload = (elem / producers);
        int producer_remainder = elem % producers;


        if (consumer_workload == 0) {
            std::cout << "WARNING: The number of consumers is greater than the number of elements to be produced. The number of consumers will be reduced from " << consumers << " to " << elem << ".\n" << std::endl;
            consumers = elem;
        } 
        if (producer_workload == 0) {
            std::cout << "WARNING: The number of producers is greater than the number of elements to be produced. The number of producers will be reduced from " << producers << " to " << elem << ".\n" << std::endl;
            producers = elem;
        }

                

        std::thread producer_setup(setup_producers, producers, elem);
     
        std::thread consumerThreads[consumers];

        
        std::thread schedulerThread;

        if (streamingMode) {
            schedulerThread = std::thread(scheduler_streaming);
        } else {
            schedulerThread = std::thread(scheduler);
        }
        
            

        for (int i = 0; i < consumers; ++i) {
            consumerThreads[i] = std::thread(consumer, i);
        }

        
        producer_setup.join();

        producersFinished = true;
        

        for (int i = 0; i < consumers; i++) {
            consumerThreads[i].join();
        }

        schedulerThread.join();


        #ifdef D_LOL

        std::vector<LogEntry> logEntries = readLogFile(logname);

        execOverview(logEntries, logname);

        #endif



    } else if (command == "-f" && argc == 4) {
        std::string pathWorkerFile = argv[2];
        std::string pathTaskFile = argv[3];
        loadworkersFile(pathWorkerFile);
        loadtaskFile(pathTaskFile);

        #ifdef D_LOL

        //std::string logname;
        std::cout << "What is the log file name?: ";
        std::cin >> logname;

        if(logname.empty()) {
            logname = "log.csv"; // Default name if empty
        }
        
        if(logname.find(".csv") == std::string::npos) {
            logname += ".csv"; // Add .csv extension if not present
        }

        #endif

        std::cout << "Does your scheduler require chunksize? (y/n): ";
        char response;
        std::cin >> response;

        if (response == 'y' || response == 'Y') {
            requiresChunkSize = true;
            std::cout << "chunksize number?: ";
            std::cin >> chunkSize;
            
        // Validate input
        while (std::cin.fail() || chunkSize <= 0) {
            std::cin.clear(); // Clear error flags
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // Skip bad input
            std::cout << "Please enter a valid positive number: ";
            std::cin >> chunkSize;
        }
    
        std::cout << "Using chunk size: " << chunkSize << std::endl;
            } else {
            std::cout << "continuing without chunksize..." << std::endl;
            requiresChunkSize = false;
            }   

        int elem = taskCount;
        int consumers = consCount;

        simulateError(false ,1, taskBuffer);


        int consumer_workload = (elem / consumers); 

        if (consumer_workload == 0) {
            std::cout << "WARNING: The number of consumers is greater than the number of elements to be consumed. The number of consumers will be reduced from " << consumers << " to " << elem << ".\n" << std::endl;
            consumers = elem;
        } 

        globalStartTime = std::chrono::high_resolution_clock::now();

        std::thread consumerThreads[consumers];

        std::thread schedulerThread(scheduler);

        #ifdef D_LOL
    
        log_precursor.push_back("Consumer ID |  Consumer Type | Task ID |  Task Type  |   Duration  |  Ellapsed Time\n");
        ++log_counter;
        log_precursor.push_back( " -------------------------------------------------------------------------\n");   
        
        #endif
            

        for (int i = 0; i < consumers; ++i) {
            consumerThreads[i] = std::thread(consumer, i);
        }


        producersFinished = true;
        

        for (int i = 0; i < consumers; i++) {
            consumerThreads[i].join();
        }

        schedulerThread.join();

    
        globalEndTime = std::chrono::high_resolution_clock::now();
        totalExecutionTime = std::chrono::duration<float>(globalEndTime.load() - globalStartTime.load()).count();
        std::cout << "Total execution time: " << totalExecutionTime << " seconds" << std::endl;

   

        #ifdef D_LOL

        std::vector<LogEntry> logEntries = readLogFile(logname);

        execOverview(logEntries, logname);

        #endif





    } else if (command == "-pred" && argc == 5) {
        std::string pathWorkerFile = argv[2];
        std::string pathTaskFile = argv[3];
        int error = std::stoi(argv[4]);

        #ifdef D_LOL

        //std::string logname;
        std::cout << "What is the log file name?: ";
        std::cin >> logname;

        if(logname.empty()) {
            logname = "log.csv"; // Default name if empty
        }
        
        if(logname.find(".csv") == std::string::npos) {
            logname += ".csv"; // Add .csv extension if not present
        }

        #endif

        std::cout << "Does your scheduler require chunksize? (y/n): ";
        char response;
        std::cin >> response;
        
        if (response == 'y' || response == 'Y') {
            requiresChunkSize = true;
            std::cout << "chunksize number?: ";
            std::cin >> chunkSize;
           
        // Validate input
        while (std::cin.fail() || chunkSize <= 0) {
            std::cin.clear(); // Clear error flags
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // Skip bad input
            std::cout << "Please enter a valid positive number: ";
            std::cin >> chunkSize;
        }
        
        std::cout << "Using chunk size: " << chunkSize << std::endl;
            } else if (response == 'n' || response == 'N'){
            std::cout << "continuing without chunksize..." << std::endl;
            requiresChunkSize = false;
            }  else {std::cout << "Nice try, try again...\n";
            return 1;} 

            if (error>100 || error<1) {
                std::cerr << "ERROR: The error must be between 1 and 100.\n";
                return 1;
            }

            std::cout << "WARNING: When using -pred all the tasks will have a prediction error of " << error << "%.\n" << std::endl;

        loadworkersFile(pathWorkerFile);
        loadtaskFile(pathTaskFile);

        int elem = taskCount;
        int consumers = consCount;

        simulateError(true, error, taskBuffer);


        int consumer_workload = (elem / consumers); 

        if (consumer_workload == 0) {
            std::cout << "WARNING: The number of consumers is greater than the number of elements to be consumed. The number of consumers will be reduced from " << consumers << " to " << elem << ".\n" << std::endl;
            consumers = elem;
        } 


        std::thread consumerThreads[consumers];

        std::thread schedulerThread(scheduler);

        #ifdef D_LOL
    
        log_precursor.push_back("Consumer ID |  Consumer Type | Task ID |  Task Type  |   Duration  |  Ellapsed Time\n");
        ++log_counter;
        log_precursor.push_back( " -------------------------------------------------------------------------\n");   
        
        #endif
            

        for (int i = 0; i < consumers; ++i) {
            consumerThreads[i] = std::thread(consumer, i);
        }


        producersFinished = true;
        

        for (int i = 0; i < consumers; i++) {
            consumerThreads[i].join();
        }

        schedulerThread.join();
        #ifdef D_LOL

        std::vector<LogEntry> logEntries = readLogFile(logname);

        execOverview(logEntries, logname);

        #endif

        


    } else if (command == "-cc" && argc == 5) {
        int consumers = std::stoi(argv[2]);
        int cpu = std::stoi(argv[3]);
        std::string pathTaskFile = argv[4];

        #ifdef D_LOL

        //std::string logname;
        std::cout << "What is the log file name?: ";
        std::cin >> logname;

        if(logname.empty()) {
            logname = "log.csv"; // Default name if empty
        }
        
        if(logname.find(".csv") == std::string::npos) {
            logname += ".csv"; // Add .csv extension if not present
        }

        #endif

        std::cout << "Does your scheduler require chunksize? (y/n): ";
        char response;
        std::cin >> response;

        if (response == 'y' || response == 'Y') {
            requiresChunkSize = true;
            std::cout << "chunksize number?: ";
            std::cin >> chunkSize;
            
        // Validate input
        while (std::cin.fail() || chunkSize <= 0) {
            std::cin.clear(); // Clear error flags
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // Skip bad input
            std::cout << "Please enter a valid positive number: ";
            std::cin >> chunkSize;
        }
    
        std::cout << "Using chunk size: " << chunkSize << std::endl;
            } else {
            std::cout << "continuing without chunksize..." << std::endl;
            requiresChunkSize = false;
            }   

        if (cpu>consumers) {
            std::cerr << "Error: The number of consumers is less than the number of CPU consumers. The number of cpu's will be reduced from " << cpu << " to " << consumers << ".\n" << std::endl;
            cpu=consumers;
        }

        #ifdef D_LOL
    
        log_precursor.push_back("Consumer ID |  Consumer Type | Task ID |  Task Type  |   Duration  |  Ellapsed Time\n");
        ++log_counter;
        log_precursor.push_back( " -------------------------------------------------------------------------\n");   
        
        #endif
        
        saveworkersFile(consumers, cpu);
        loadworkersFile("workers.txt");
        loadtaskFile(pathTaskFile);

        int elem = taskCount;

        int consumer_workload = (elem / consumers); 

        if (consumer_workload == 0) {
            std::cout << "WARNING: The number of consumers is greater than the number of elements to be produced. The number of consumers will be reduced from " << consumers << " to " << elem << ".\n" << std::endl;
            consumers = elem;
        } 


        std::thread consumerThreads[consumers];

        std::thread schedulerThread(scheduler);
            

        for (int i = 0; i < consumers; ++i) {
            consumerThreads[i] = std::thread(consumer, i);
        }


        producersFinished = true;


        for (int i = 0; i < consumers; i++) {
            consumerThreads[i].join();
        }

        schedulerThread.join();
        #ifdef D_LOL

        std::vector<LogEntry> logEntries = readLogFile(logname);

        execOverview(logEntries, logname);

        #endif

    } else if (command == "-cp" && argc == 5 || argc == 6) {
        int elem = std::stoi(argv[2]);
        std::string pathWorkerFile = argv[3];
        int producers = std::stoi(argv[4]);
        bool streamingMode = (argc == 6 && std::string(argv[5]) == "stream");

        #ifdef D_LOL

        //std::string logname;
        std::cout << "What is the log file name?: ";
        std::cin >> logname;

        if(logname.empty()) {
            logname = "log.csv"; // Default name if empty
        }
        
        if(logname.find(".csv") == std::string::npos) {
            logname += ".csv"; // Add .csv extension if not present
        }

        #endif
        
        std::cout << "Does your scheduler require chunksize? (y/n): ";
        char response;
        std::cin >> response;

        if (response == 'y' || response == 'Y') {
            requiresChunkSize = true;
            std::cout << "chunksize number?: ";
            std::cin >> chunkSize;
            
        // Validate input
        while (std::cin.fail() || chunkSize <= 0) {
            std::cin.clear(); // Clear error flags
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // Skip bad input
            std::cout << "Please enter a valid positive number: ";
            std::cin >> chunkSize;
        }
    
        std::cout << "Using chunk size: " << chunkSize << std::endl;
            } else {
            std::cout << "continuing without chunksize..." << std::endl;
            requiresChunkSize = false;
            }   


        if (streamingMode) {
            std::cout << "Using streaming mode: producers and scheduler run in parallel\n";
        }

        useProducer=true;
        

        std::ofstream clearFile("tasksProduced.txt", std::ios::trunc);
        clearFile.close();

        loadworkersFile(pathWorkerFile);

        #ifdef D_LOL
    
        log_precursor.push_back("Consumer ID |  Consumer Type | Task ID |  Task Type  |   Duration  |  Ellapsed Time\n");
        ++log_counter;
        log_precursor.push_back( " -------------------------------------------------------------------------\n");   
        
        #endif
        int consumers = consCount;

        int consumer_workload = (elem / consumers); 
        int producer_workload = (elem / producers);
        int producer_remainder = elem % producers;


        if (consumer_workload == 0) {
            std::cout << "WARNING: The number of consumers is greater than the number of elements to be produced. The number of consumers will be reduced from " << consumers << " to " << elem << ".\n" << std::endl;
            consumers = elem;
        } 
        if (producer_workload == 0) {
            std::cout << "WARNING: The number of producers is greater than the number of elements to be produced. The number of producers will be reduced from " << producers << " to " << elem << ".\n" << std::endl;
            producers = elem;
        }



        std::thread producer_setup(setup_producers, producers, elem);

        std::thread consumerThreads[consumers];

        std::thread schedulerThread;

        if (streamingMode) {
            schedulerThread = std::thread(scheduler_streaming);
        } else {
            schedulerThread = std::thread(scheduler);
        }

        for (int i = 0; i < consumers; ++i) {
            consumerThreads[i] = std::thread(consumer, i);
        }

        

        producer_setup.join();

        producersFinished = true;

        //loadtaskFile("tasksProduced.txt");
        
        tasksLoaded = true;
        
              
        for (int i = 0; i < consumers; i++) {
            consumerThreads[i].join();
        }

        schedulerThread.join();
        #ifdef D_LOL

        std::vector<LogEntry> logEntries = readLogFile(logname);

        execOverview(logEntries, logname);

        #endif
       
    } else if (command == "-help" && argc == 2) {
        usage();
    } else {
        std::cerr << "Error: Invalid command or incorrect number of arguments.\n";
        usage();
        return 1;
    }


    return 0;
}
