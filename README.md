# tese

Ficheiros de código úteis na tese.

Os ficheiros no prototype são os mais recentes e instáveis.

SchedulerSW.h é o scheduler com workstealing, neste momento só funciona com o lockfree.cpp do prototype

O heft começa por criar uma DAG que guarda o custo de comunicação das tasks e calcul aos parents de cada task, depois vê o computation cost (task temp * consumer freq) e guarda num array. Depois dá assign a um rank às tasks com base na informação previamnete calculada e por fim dá sort ao rank e distribui cada task para o melhor consumer consoante a disponibilidade.

// g++ -g LockFree.cpp -O3 -o engine -lpthread -I/home/hondacivic/Boost/boost_1_82_0 -DD_LOL


