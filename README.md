# tese

Ficheiros de código úteis na tese

Tentei fazer uma espécie de mutex só com OpenMP, mas ainda não funciona como pretendido.

O mutex e o semaphore estão atualizado para uma versão que funciona direito (acho eu).

A base de funcionamento é assumir que todos os workers têm a mesma eficiência e por isso dividir igualmente as tasks, quando houver um número de tasks em que a divisão é decimal o resto da divisão fica para o ultimo worker.
