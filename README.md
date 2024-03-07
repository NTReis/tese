# tese

Ficheiros de código úteis na tese.

A base de funcionamento é distribuir igualmente pelos consumers e producers, quando não dá para o fazer distribuiu o resto por ordem até deixar de haver tasks a mais.

Quando fazemos testagem com Consumer.h e criamos a base de dados dos consumers no ficheiro .txt o número de consumers tem qeu ser estático não pode variar.
Temos que correr sempre um ciclo para criar e só deposi se pode realizar a testagem.

Tenho que investigar se houver uma maneira para dar load uma vez e não testar mais, mas duvido.

Agora o consumer function só recebe o ID e o Consumer consumer inicializado na main. depois pode fazer os acessos aí. A maneira como funciona agora para saber quanto falta em vez de calcular tudo de novo o scheduler mete num vector quanto cada consumer tem qeu consumir e depois o consumer com um pointer chega até esse vector que foi criado na main e sabe quanto tem que consumir. No final da main dou delete ao vector para libertar o espaço alocado de memória.

