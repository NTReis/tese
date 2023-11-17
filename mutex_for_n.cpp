#include <iostream>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

std::atomic<bool> done(false);

//sacar coisas da standard library
std::queue<int> buffer; //declara uma queue de ints chamada buffer
std::mutex mtx; //declara um mutex chamado mtx
std::condition_variable cv; //declara uma variavel de condição chamada cv. 
//Uma variável de condição é um objeto capaz de bloquear a thread até que uma condição seja satisfeita. 
//Ela é sempre usada em conjunto com um mutex, pois a condição é verificada enquanto o mutex está bloqueado.



void producer() {

   int n;
   std::cout << "How many elements do you want to produce? ";
   std::cin >> n;
   std::cout << "The value of x is: " << n << std::endl;


   for (int i = 1; i <= n; ++i) {
      std::lock_guard<std::mutex> lock(mtx); //lockguard é um objeto que gerencia o mutex. Isto faz com que só um thread possa usar o mutex por vez.
      buffer.push(i); //push adiciona um elemento no fim da queue  
      std::cout << "Produced: " << i << std::endl; //é como se fosse o printf do C
      cv.notify_one(); //notifica um thread que está esperando na variavel de condição que existe data para ser consumida
      std::this_thread::sleep_for(std::chrono::milliseconds(200)); //faz o thread dormir por 500 milisegundos
   }
   done = true;
}

void consumer() {

   int i = 0;
   
   while (true) { //má pratica
      std::unique_lock<std::mutex> lock(mtx);
      if (buffer.empty() && done) {
            break; // exit if buffer is empty and done flag is true
        }
      cv.wait(lock, [] { return !buffer.empty(); }); //o lock esta com o problema bloquear os outros consumidores. Tenho que tirar o cout dos lock

      //wait faz com que o thread espere até que a variavel de condição seja notificada.
      //O primeiro argumento é o mutex, o segundo é uma função lambda que retorna true se a condição for satisfeita.
      //[] { return !buffer.empty(); } esta função lambda verifica se a queue está vazia. Se estiver, o thread fica à espera até que o produtor adicione algo na queue e notifique a variavel de condição.
      
      int data = buffer.front(); //dá store na data do primeiro elemento 
      buffer.pop(); //remove o primeiro elemento da queue com pop

      std::cout << "Consumer " <<  i  << ": " << data << std::endl; // imprime a data

      lock.unlock(); //dar unlokc permite com que outros threads possam dar lock e usar o mutex


      std::this_thread::sleep_for(std::chrono::milliseconds(1000));

      ++i;
   }

}

int main() {

    int n;
    std::cout << "How many consumers do you want? ";
    std::cin >> n; 
    //std::cout << "The value of consumers is: " << n << std::endl;

    std::thread producerThread(producer); //define os threads de produtor e consumidor

    std::thread consumerThreads[n]; //criar um array com n threads de consumidor


    for(int i = 0; i < n; i++){
        consumerThreads[i] = std::thread(consumer); // pass i as the id argument
    }
    
    producerThread.join(); //faz o programa esperar até que os threads terminem 


    for (int i = 0; i < n; i++){
        consumerThreads[i].join();
}

    return 0;
}

//este código anda de 4 em 4 threads e assumo que seja sempre um consumidor a mudar
//deve dar para fazer com vetores mas nao percebo muito bem 
//não sei como terminar para o programa sem ser com o ctr+c
