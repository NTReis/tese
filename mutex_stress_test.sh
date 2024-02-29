#!/bin/bash

# Compile the C++ program
g++ teste_para_script.cpp -o mutex_exec -lpthread

num_consumers=$(( (RANDOM % 200) + 1 ))  # Random number between 1 and 10
num_producers=$(( (RANDOM % 200) + 1 ))  # Random number between 1 and 20
num_elements=$(( (RANDOM % 200) + 1 ))  # Random number between 1 and 200
cycles=10


# Run the program in a loop for a certain number of cycles
for (( cycle = 1; cycle <= cycles; cycle++ )); do
    echo "Running test cycle $cycle with $num_consumers consumers, $num_producers producers and $num_elements elements"

    sleep 1

    # Run the C++ program with specified parameters
    ./mutex_exec "$num_consumers" "$num_producers" "$num_elements"


    echo "----------------------------------------"
    echo -e "\n"

    sleep 1

done
