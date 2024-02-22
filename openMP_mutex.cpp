#include <iostream>
#include <queue>
#include <thread>
#include <omp.h>

//std::mutex mtx; //declara um mutex chamado mtx
//std::condition_variable cv; //declara uma variavel de condição chamada cv. 

std::queue<int> buffer; 
int buffer_empty = 1;   // talvez use isto para ver se o buffer está relaemnte vazio

void producer(int elem) {
#pragma omp parallel for
    for (int i = 1; i <= elem; ++i) {
        int produced;
#pragma omp critical
        {
            buffer.push(i);
            produced = i;
            buffer_empty = 0; // Signal that the buffer is not empty
        }
        std::cout << "Produced: " << produced << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    buffer_empty = 1; // Signal that production is done
}

void consumer(int id, int num_items) {
    int i = 0;

    while (i < num_items) {
        int data;
#pragma omp critical
        {
            if (!buffer.empty()) {
                data = buffer.front();
                buffer.pop();
                buffer_empty = buffer.empty(); //true or false
            }
        }

        if (!buffer_empty) {
            std::cout << "Consumer " << id << ": " << data << std::endl;
            ++i;
        }
    }
}

int main() {
    int n;
    std::cout << "How many consumers do you want? ";
    std::cin >> n;

    int elem;
    std::cout << "How many elements do you want to produce? ";
    std::cin >> elem;

    int divid = (elem / n);
    int resto = elem % n; // not working

#pragma omp parallel sections
    {
#pragma omp section
        {
            producer(elem);
        }

#pragma omp section
        {
            for (int i = 0; i < n - 1; ++i) {
                consumer(i, divid);
            }
            if (resto == 0) {
                consumer(n - 1, divid);
            } else {
                consumer(n - 1, divid + resto);
            }
        }
    }

    return 0;
}
