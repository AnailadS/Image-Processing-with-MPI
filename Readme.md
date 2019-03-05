	Image Processing with MPI

	This program can apply any number of filters on a .pnm or .pgm image.

	The filters that can be applied are :
		"smooth"  - Smoothing filter
		"blur"    - Approximative Gaussian Blur filter
		"sharpen" - Sharpen filter
		"mean"    - Mean removal filter
		"emboss"  - Emboss filter

	Compile from command line:
		mpicc -o imageProcessor imageProcessor.c

	Run from command line:
		mpirun -np N imageProcessor image_in.pnm image_out.pnm filter1 filter2 filter3
	N = number of processes

