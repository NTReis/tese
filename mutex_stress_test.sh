#!/bin/bash

# Compile the C++ program
g++ teste_para_script.cpp -o mutex_exec -lpthread

# Set parameters
num_consumers=4
num_elements=27
cycles=3

# Run the program in a loop for a certain number of cycles
for (( cycle = 1; cycle <= cycles; cycle++ )); do
    echo "Running test cycle $cycle with $num_consumers consumers and $num_elements elements"

    sleep 1

    # Run the C++ program with specified parameters
    ./mutex_exec "$num_consumers" "$num_elements"


    echo "----------------------------------------"
    echo -e "\n"

    sleep 1

done
