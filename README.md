# tese

Ficheiros de código úteis na tese.

A base de funcionamento é distribuir igualmente pelos consumers e producers, quando não dá para o fazer distribuiu o resto por ordem até deixar de haver tasks a mais.


O mutex_stress_test.sh e o teste_para_script.cpp são ficheiros para testagem extensa quando aumento o tempo de sleep das irregular tasks fica tudo desincronizado.
Mudei o queue para dqueue que pode aumentar e diminuir dinamiocamente, também aloca automaticamente o espaço de memória necessário.
