# CBACO-Yahui

## Source

This project is associated with the following papers:

```bibtex
@article{jia2021bilevel,
  title={A bilevel ant colony optimization algorithm for capacitated electric vehicle routing problem},
  author={Jia, Ya-Hui and Mei, Yi and Zhang, Mengjie},
  journal={IEEE transactions on cybernetics},
  volume={52},
  number={10},
  pages={10855--10868},
  year={2021},
  publisher={IEEE}
}

@article{jia2022confidence,
  title={Confidence-based ant colony optimization for capacitated electric vehicle routing problem with comparison of different encoding schemes},
  author={Jia, Ya-Hui and Mei, Yi and Zhang, Mengjie},
  journal={IEEE Transactions on Evolutionary Computation},
  volume={26},
  number={6},
  pages={1394--1408},
  year={2022},
  publisher={IEEE}
}
```

## Build

```sh
cmake -S . -B build
cmake --build build
```

## Run

Place EVRP instance files in the `instances/` directory, then run the solver from the `build/` directory:

```sh
cd build
./CEVRP-Yahui baco "\$CASE"  0  # baco - max evals
./CEVRP-Yahui baco "\$CASE"  1  # baco - max time
./CEVRP-Yahui cbaco "\$CASE" 0  # cbaco-i - max evals
./CEVRP-Yahui cbaco "\$CASE" 1  # cbaco-i - max time
```

The executable now runs 10 trials simultaneously using seeds `1` through `10`.

## Output

For an instance named `E-n22-k4.evrp`, results are written under:

```text
stats/baco/E-n22-k4/
stats/cbaco/E-n22-k4/
```

This directory contains:

- `stats.E-n22-k4.txt`: summary statistics over the 10 runs
- `1/` ... `10/`: one subdirectory per seed
- `solution.E-n22-k4.txt`: best solution found for that seed
- `evols.E-n22-k4.csv`: objective value over time/evaluations for that seed

Example output layout:

```text
stats/baco/E-n22-k4/
  stats.E-n22-k4.txt
  1/
    solution.E-n22-k4.txt
    evols.E-n22-k4.csv
  2/
  ...
  10/
```

