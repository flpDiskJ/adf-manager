:ADF Manager:

Simple CLI program for managing Amiga ADF files (Double Density)

Add and extract files, and create empty ADFs

Code by Jake Aigner

Uitilizing ADFlib https://github.com/lclevy/ADFlib

If you have installed the deb package simply run "adfman" in a terminal.

To compile run the following:

	mkdir build  # create build dir
	
	cd build     # nav into build dir
	
	cmake ..     # generate makefile
	
	make         # compile program
	
	./adfman     # run program
	
	run "cpack" to generate deb package
