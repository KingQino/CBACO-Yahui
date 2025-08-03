# CEVRP

Using BACO to solve the basic CEVRP. Please read "Jia, Y. H., Mei, Y., & Zhang, M. (2021). A Bilevel Ant Colony Optimization Algorithm for Capacitated Electric Vehicle Routing Problem. IEEE Transactions on Cybernetics." for the explanation of the algorithm.

Using CACO to solve the basic CEVRP with comparison of different encoding schemes. Please read "Jia, Y. H., Mei, Y., & Zhang, M. (2022). Confidence-based Ant Colony Optimization for Capacitated Electric Vehicle Routing Problem with Comparison of Different Encoding Schemes. IEEE Transactions on Evolutionary Computation." for the explanation of the algorithm.



> The main changes made by me is on three aspects:
>
> - Max-Evals stop criteria (aligns with the competition benchmark stop criteria)
> - Statistical function
> - Multithreading running

1. run command

   ```sh
   ./CEVRP-Yahui baco "\$CASE"  0  # baco - max evals
   ./CEVRP-Yahui baco "\$CASE"  1  # baco - max time
   ./CEVRP-Yahui cbaco "\$CASE" 0  # cbaco-i - max evals
   ./CEVRP-Yahui cbaco "\$CASE" 1  # cbaco-i - max time
   ```

   
