# CEVRP

Using BACO to solve the basic CEVRP. Please read "Jia, Y. H., Mei, Y., & Zhang, M. (2021). A Bilevel Ant Colony Optimization Algorithm for Capacitated Electric Vehicle Routing Problem. IEEE Transactions on Cybernetics." for the explanation of the algorithm.

Using CACO to solve the basic CEVRP with comparison of different encoding schemes. Please read "Jia, Y. H., Mei, Y., & Zhang, M. (2022). Confidence-based Ant Colony Optimization for Capacitated Electric Vehicle Routing Problem with Comparison of Different Encoding Schemes. IEEE Transactions on Evolutionary Computation." for the explanation of the algorithm.



> The main changes made by me is on three aspects:
>
> - Max-Evals stop criteria (aligns with the competition benchmark stop criteria)
> - Statistical function
> - Multithreading running

- Usage:

1.  `setup.sh`

   ```sh
   #!/bin/bash
   
   # Target directories
   directories=$(ls -d */ | grep -Ev '^(A|baco)/')
   
   # Check if any directories were found
   if [ -z "$directories" ]; then
       echo "No directories found."
       exit 1
   fi
   
   # Loop through each directory to set up build structure and create script.slurm
   for dir in $directories; do
       # Remove trailing slash from directory name
       dir=${dir%/}
   
       # Check if the directory exists
       if [ -d "$dir" ]; then
           # Define paths for build and log folders
           build_dir="$dir/build"
           log_dir="$build_dir/log"
   
           # Create the build and log directories if they don't exist
           mkdir -p "$log_dir"
           echo "Created '$log_dir'."
   
           # Copy parameters.txt into the build folder
           if [ -f "parameters.txt" ]; then
               cp parameters.txt "$build_dir"
               echo "Copied 'parameters.txt' to '$build_dir'."
           else
               echo "'parameters.txt' not found in the current directory."
           fi
   
           # Create script.slurm with dynamic content
           cat > "$build_dir/script.slurm" <<EOL
   #!/bin/bash
   
   # Slurm job options (job-name, compute nodes, job time)
   #SBATCH --job-name=Ya-$dir                             # Job name set to the parent directory name
   #SBATCH --output=$(pwd)/$log_dir/slurm-%A_%a.out       # Output log file path in the log folder
   #SBATCH --time=48:0:0                                  # Request 48 hours of compute time
   #SBATCH --nodes=1                                      # Request 1 node
   #SBATCH --tasks-per-node=1                             # One task per node
   #SBATCH --cpus-per-task=10                             # Each task uses 10 CPUs (threads)
   #SBATCH --mem-per-cpu=4G                               # Memory per CPU
   #SBATCH --account=su008-exx866
   #SBATCH --array=0-16
   
   # Load necessary modules
   module load GCCcore/13.3.0 GCC/13.3.0 OpenMPI/5.0.3
   
   
   # Load cases from parameters.txt
   mapfile -t cases < "parameters.txt"        # Load parameters.txt from the build directory
   CASE="\${cases[\$SLURM_ARRAY_TASK_ID]}"
   
   # Run the specified command with case argument
   srun ./CEVRP-Yahui cbaco "\$CASE" 1 
   EOL
   
           echo "Generated 'script.slurm' in '$build_dir'."
   
           # Navigate to build_dir, run cmake and make commands
           (
               cd "$build_dir" || exit
               ml GCCcore/13.3.0 CMake/3.29.3 GCC/13.3.0 OpenMPI/5.0.3
               cmake  ..
               make
               sbatch script.slurm
           )
       else
           echo "Directory '$dir' does not exist."
       fi
   done
   
   ```

2. run command

   ```sh
   ./CEVRP-Yahui baco "\$CASE"  0  # baco - max evals
   ./CEVRP-Yahui baco "\$CASE"  1  # baco - max time
   ./CEVRP-Yahui cbaco "\$CASE" 0  # cbaco-i - max evals
   ./CEVRP-Yahui cbaco "\$CASE" 1  # cbaco-i - max time
   ```

   
