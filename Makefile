build: imageProcessor
imageProcessor: imageProcessor.c
	mpicc -o imageProcessor imageProcessor.c -lm -Wall
serial: imageProcessor
	mpirun -np 1 imageProcessor in/lenna_color.pnm lenna_out.pnm blur sharpen
distrib: imageProcessor
	mpirun -np 4 imageProcessor in/lenna_color.pnm lenna_out.pnm blur sharpen
clean:
	rm -f imageProcessor
