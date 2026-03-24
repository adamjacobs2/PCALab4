#!/bin/bash
#SBATCH --account=eel6763
#SBATCH --qos=eel6763
#SBATCH --nodes=1
#SBATCH --ntasks=4
#SBATCH --ntasks-per-node=4
#SBATCH --cpus-per-task=4
#SBATCH --mem-per-cpu=1000mb
#SBATCH -t 00:05:00
#SBATCH -o 1Nodes_4Ranks_closed
#SBATCH -e errfile
export OMP_NUM_THREADS=4
export OMP_PROC_BIND=spread
srun --mpi=$HPC_PMIX ./a.out
