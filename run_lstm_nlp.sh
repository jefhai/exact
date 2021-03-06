#!/bin/bash -l
# NOTE the -l flag!
#

#SBATCH -J examm_lstm_nlp

#SBATCH -A examm    #just examm examm_thompson is not a valid account

#SBATCH -o cluster_logs/examm_test_%A.output
#SBATCH -e cluster_logs/examm_test_%A.error

#SBATCH --mail-user vj6810@rit.edu

#SBATCH --mail-type=ALL

#SBATCH -t 5-0     # if you want to run for 4 days 1-72:0:0

# Put the job in the "work" partition and request FOUR cores for one task
# "work" is the default partition so it can be omitted without issue.

## Please not that each node on the cluster is 36 cores
#SBATCH -p tier3 -n 4


# Job memory requirements in MB
#SBATCH --mem-per-cpu=4G  # I like to mem with a suffix [K|M|G|T] 5000

#module load module_future
#module load openmpi-1.10-x86_64
module load openmpi
module load gcc
module load libtiff


EXAMM="/home/vj6810/EXACT/exact_/exact"

for folder in 0 1 2 3 4 ; do
    exp_name="$EXAMM/nlp_results_1/$folder"
    mkdir -p $exp_name
    echo "\t Iteration: $exp_name"
    $EXAMM/build/multithreaded/examm_nlp --number_threads 9 \
    --training_filenames ../datasets/pennchar/train.txt \
    --test_filenames ../datasets/pennchar/valid.txt \
    --word_offset 1 \
    --number_islands 10 \
    --population_size 10 \
    --max_genomes 2000 \
    --bp_iterations 10 \
    --output_directory $exp_name \
    --possible_node_types simple LSTM \
    --normalize min_max \
    --std_message_level INFO \
    --file_message_level INFO
    
done