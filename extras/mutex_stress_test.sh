#!/bin/bash

# Compile the C++ program
#g++ teste_para_script.cpp -o mutex_exec -lpthread
g++ testing.cpp -o mutex_exec -lpthread

num_consumers=5
#num_producers=3
#num_elements=6

cycles=1


# Run the program in a loop for a certain number of cycles
for (( cycle = 1; cycle <= cycles; cycle++ )); do

    # num_consumers=$(( (RANDOM % 10) + 1 ))  
    num_producers=$(( (RANDOM % 10) + 1 ))  
    num_elements=$(( (RANDOM % 20) + 1 ))   

    echo "Running test cycle $cycle with $num_consumers consumers, $num_producers producers and $num_elements elements"

    

    #sleep 0.5

    # Run the C++ program with specified parameters
    ./mutex_exec "$num_consumers" "$num_producers" "$num_elements"


    echo "----------------------------------------"
    echo -e "\n"

    #sleep 0.5

done
