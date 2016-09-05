#!/bin/bash

# Generate the initial conditions if they are not present.
echo "Generating initial conditions for the cooling box example..."

python makeIC.py 10

../swift -s -t 1 coolingBox.yml -C

python energy_plot.py 0

python add_energy_column
