# tese

Ficheiros de código úteis na tese.

Os ficheiros no prototype são os mais recentes e alguns instáveis.

SchedulerSW.h é o scheduler com workstealing, neste momento tem um erro.

O heft começa por criar uma DAG que guarda o custo de comunicação das tasks e calcula os parents de cada task, depois vê o computation cost (task temp * consumer freq) [ver se preciso de alterar] e guarda num array. Depois dá assign a um rank às tasks com base na informação previamente calculada e por fim dá sort ao rank e distribui cada task para o melhor consumer consoante a disponibilidade.

// g++ -g LockFree.cpp -O3 -o engine -lpthread -I/home/hondacivic/Boost/boost_1_82_0 -DD_LOL


