#Simulations

This folder contains the the simulation scripts used in the work.

## Contiki node files

Contki node source files are in the folders: `client`, `server`, `border-client` and `direct_server`.

## NS-3 simulation files

NS-3 simulation files are contained in the folder `ns3/scratch`. The `ns3` folder is copyied to the container ns-3 folder.

## Scripts

### Running scripts

The main script for running a set of simulations is `cooja_increment.sh`. It creates a folder for the simulation with a cooja simulation based on one provided .csc template and the provided arguments. After running a simulation it also executes the post-processing scripts, and increments the seed value for future simulations.

`script.conf` creates and configures a simulation and is called by `cooja_increment.sh`.

`clean_sim` cleans one simulation of a given name.

`sim-ns3cooja` creates a screen to run the simulation by calling `cooja_increment.sh` with the same arguments. Contains a hard coded limit to throttle the number of concurrent simulations according to the number of processors.

`sim-force` cleans a simulation with a given name and runs it again.

### Post processing scripts

`final.awk` averages multiple simulation results.

`parse_tap.py` gets time and delivery between tap interfaces.

### Cooja scripts

`sim_script.js` runs a simulation and retrieves delivery and average values for traffic.

`sim_script_fast.js` the same as the previous, but has a lower timeout.

## Makefile

The Makefile contains two main targets: 

* `sim`: Starts one container with one simulation by executing the `ns3cooja` binary.
* `bash`: Starts one container with a bash for debug and to run separate components.

### Running ai single simulations with a Makefile

In order to run a simulation the following macros should be set:

* `SIM`: The name of the simulation; there should be a folder with this name containing a *.csc simulation with the same name.
* `NS3_SIM`: The name of the ns-3 simulation; the equivalent *.cc simulation should be in the `ns3/scratch` folder.
* `NS3_SEED` (optional): the seed to use in the ns-3 simulation; will be available as the argument `seed` in the simulation.
* `NS3_ARGS` (optional): additional arguments to be delivered to the ns-3 simulation.

The Makefile is used in the simulation script `cooja_iteration.sh`.

