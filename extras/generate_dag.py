import random
from collections import defaultdict

def generate_communication_cost_dag(num_tasks=50000, max_cost=15):
    dag = defaultdict(dict)
    edge_probability = 0.3
    for i in range(num_tasks):
        for j in range(i + 1, num_tasks, 4):
            if random.random() < edge_probability:
                cost = random.randint(1, max_cost)
                dag[i][j] = cost
    return dag

def write_dag_as_edgelist(dag, num_tasks):
    with open(f"DAG_{num_tasks}_edgelist.csv", "w") as file:
        for src in dag:
            for dst, cost in dag[src].items():
                file.write(f"{src},{dst},{cost}\n")

num_tasks = 200000
dag = generate_communication_cost_dag(num_tasks=num_tasks)
write_dag_as_edgelist(dag, num_tasks)
