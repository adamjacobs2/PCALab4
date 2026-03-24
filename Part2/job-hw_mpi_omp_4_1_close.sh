#!/bin/bash
#SBATCH --account=eel6763
#SBATCH --qos=eel6763
#SBATCH --nodes=4
#SBATCH --ntasks=4
#SBATCH --ntasks-per-node=1
#SBATCH --cpus-per-task=4
#SBATCH --mem-per-cpu=1000mb
#SBATCH -t 00:05:00
#SBATCH -o 4Nodes_1Ranks
#SBATCH -e errfile
export OMP_PROC_BIND=close
export OMP_NUM_THREADS=4
srun --mpi=$HPC_PMIX ./a.out
