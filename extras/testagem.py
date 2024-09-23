import os
import subprocess

num_execucoes = 10 
parametro_inicial = 2  
parametro_final = 16  
pasta_saida = "Testing"  


os.makedirs(pasta_saida, exist_ok=True)


for i in range(parametro_inicial, parametro_final, 2):
    comando = f"./engine -f {i}workers.txt tasksPY1000.txt"
    
    subprocess.run(comando, shell=True)
    
    nome_ficheiro = f"log.csv"
    
    if os.path.exists(nome_ficheiro):
        novo_nome_ficheiro = os.path.join(pasta_saida, f"log_{i}workers.csv")
        os.rename(nome_ficheiro, novo_nome_ficheiro)

print("Execuções completas. Resultados guardados em:", pasta_saida)
