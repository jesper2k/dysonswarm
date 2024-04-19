# Dyson Swarm
## TDT4230 Graphics and Visualization

This is the code for my project in the class TDT4230 Graphics and Visualisation, Spring 2024 at NTNU. The program simulates several configurations of a dyson swarm, which is a megastructure designed to harness the power of a star.


![swarmprogress(tiny)17](https://github.com/jesper2k/dysonswarm/assets/55808963/d37765fc-3bdc-4018-b954-2a376da99992)


Other students' projects for previous years can be seen [here](https://www.idi.ntnu.no/grupper/vis/teaching/)




## What do i do?

	git clone --recursive https://github.com/bartvbl/TDT4230-Assignment-1.git

Should you forget the `--recursive` bit, just run:

	git submodule update --init


### Windows

Install Microsoft Visual Studio Express and CMake.
You may use CMake-gui or the command-line cmake to generate a Visual Studio solution.

### Linux:

Make sure you have a C/C++ compiler such as  GCC, CMake and Git.

	make run

which is equivalent to

	git submodule update --init
	cd build
	cmake ..
	make
	./glowbox
