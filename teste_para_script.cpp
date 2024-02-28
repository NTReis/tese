#include <iostream>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <unistd.h>
#include <random>

class Tasks {
public:
    int duration;
    bool regular;

    // Constructor
    Tasks() {
        setRandomRegular();
    }

    void run() {
        if (regular) {
            std::cout << "Regular task\n";
            //usleep(200);
        } else {
            std::cout << "Irregular task: ";
            std::random_device rd;
            std::mt19937 gen(rd());
            std::normal_distribution<double> distribution(0.51, 0.5);

            double random_value = distribution(gen);
            //usleep(static_cast<int>(random_value * 200));

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

std::queue<Tasks> taskBuffer;
std::mutex mtx;
std::condition_variable cv;

void producer(int elem) {
    for (int i = 1; i <= elem; ++i) {
        int produced;
        {
            std::lock_guard<std::mutex> lock(mtx);
            taskBuffer.push(Tasks());
            produced = i;
            cv.notify_one();
        }
        std::cout << "Produced: " << produced << std::endl;
        //std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    cv.notify_all();
}

void consumer(int id, int num_items) {
    int i = 0;

    while (i < num_items) {
        std::unique_lock<std::mutex> lock(mtx);

        cv.wait(lock, [] { return !taskBuffer.empty(); });

        if (!taskBuffer.empty()) {
            Tasks task = taskBuffer.front();
            taskBuffer.pop();
            task.run();
            lock.unlock();
            std::cout << "Consumer " << id << std::endl;
        }
        ++i;
    }
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <num_consumers> <num_elements>\n";
        return 1;
    }

    int n = std::stoi(argv[1]);
    int elem = std::stoi(argv[2]);


    int divid = (elem / n); // Number of elements to be consumed by each consumer

    if (divid == 0) {
         std::cout << "WARNING: The number of consumers is greater than the number of elements to be produced. The number of consumers will be reduced from " << n << " to " << elem << "\n "<< std::endl;
         divid = 1;
    }

    std::thread producerThread(producer, elem);

    std::thread consumerThreads[n];


   // Caso o número de consumers seja maior que o número de tasks, inicializo só os necessários
   //Não sei se quando temos mais consumers do que tasks se inicializo só os necessários ou inicializo os outros com 0 tasks

    int resto = 0;

    for (int i = 0; i < n - 1; ++i) {
      consumerThreads[i] = std::thread(consumer, i, divid);
      resto += divid;
   }
   
   consumerThreads[n - 1] = std::thread(consumer, n - 1, elem - resto);
   
   producerThread.join();

    for (int i = 0; i < n; i++) {
        consumerThreads[i].join();
    }

    return 0;
}
