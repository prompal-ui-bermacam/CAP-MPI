#!/bin/bash

nodes=(1 2 4 8)
matrix_sizes=(256 512 1024 2048 4096)
max_execs=3

for num_nodes in "${nodes[@]}"
do
  for size in "${matrix_sizes[@]}"
  do
    num_exec=0
    while [[ $num_exec -lt $max_execs ]]
    do
      echo "------------------------------------------"
      echo "| np=$num_nodes ; size=$size ; $num_exec |"
      echo "------------------------------------------"
      mpirun --allow-run-as-root -np $num_nodes ./gs_mpi $size 0
      num_exec=$(( num_exec+1 ))
    done
  done
done
