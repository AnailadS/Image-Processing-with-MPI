#include<mpi.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>

unsigned char smoothing(int i, int j, unsigned char **img, int step)
{
	long sum = 0;
	float f = 1.0/9.0;
	sum = img[i - 1][j - step] * f  + img[i - 1][j] * f + img[i - 1][j + step] * f
			+ img[i][j - step] * f + img[i][j]  * f + img[i][j + step] * f
			+ img[i + 1][j - step] * f + img[i + 1][j] * f + img[i + 1][j + step] * f;
	unsigned char p = sum;
	return p;	
}

unsigned char emboss(int i, int j, unsigned char **img, int step)
{
	long sum = 0;
	sum = img[i - 1][j] - img[i + 1][j];
	unsigned char p = sum;
	return p;	
}

unsigned char gaussian(int i, int j, unsigned char **img, int step)
{
	long sum = 0;
	sum = img[i - 1][j - step] + img[i - 1][j] * 2 + img[i - 1][j + step]
			+ img[i][j - step] * 2 + img[i][j]  * 4 + img[i][j + step] * 2
			+ img[i + 1][j - step] + img[i + 1][j] * 2 + img[i + 1][j + step];
	sum = sum / 16;
	unsigned char p = sum;
	return p;	
}

unsigned char sharpen(int i, int j, unsigned char **img, int step)
{
	long sum = 0;
	float f1 = 1.0/3.0 * -2.0;
	float f2 = 1.0/3.0 * 11.0;
	sum = img[i - 1][j] * f1 + img[i][j - step] * f1 + img[i][j]  * f2
		 + img[i][j + step] * f1 + img[i + 1][j] * f1;
	unsigned char p = sum;
	return p;	
}

unsigned char mean_removal(int i, int j, unsigned char **img, int step)
{
	long sum = 0;
	sum = - img[i - 1][j - step]  - img[i - 1][j] - img[i - 1][j + step]
			- img[i][j - step] + img[i][j] * 9 - img[i][j + step]
			- img[i + 1][j - step] - img[i + 1][j] - img[i + 1][j + step];
	unsigned char p = sum;
	return p;	
}

void read_input(char *in, int *numcolors, int *height, int *width, int *maxval, unsigned char ***data)
{
	//Read Input
	char *str = calloc(1, 100);
	FILE *infile = fopen(in, "rb");
	fgets(str, 100, infile);
	if(str[1] == '6')
		*numcolors = 3;
	else
		*numcolors = 1; 

	fgets(str, 100, infile);
	char *h, *w;
	w = strtok(str, " ");
	*width = atoi(w);
	h = strtok(NULL, "\n");
	*height = atoi(h);

	fgets(str, 100, infile);
	char *m = strtok(str, "\n");
	*maxval = atoi(m);

	/*read the pixels*/
	*data = malloc(*height * sizeof(unsigned char *));
	 for (int i = 0 ; i < *height ; i++)
	 {
	 	(*data)[i] = malloc(*numcolors * (*width) * sizeof(unsigned char));
	 	fread((*data)[i] , *numcolors * (*width), 1, infile);
	 }

	free(str);
	fclose(infile);

}

void write_output(char *out, int numcolors, int height, int width, int maxval, unsigned char **data)
{
	FILE *outfile = fopen(out, "wb");
	char *str = calloc(1, 100);

	/*write the type of the file*/
	if (numcolors == 1)
		fputs("P5\n", outfile);
	else
		fputs("P6\n", outfile);

	/*write the height and the width of the file*/
	snprintf(str, 20, "%d ", width);
	fputs(str, outfile);
	snprintf(str, 20, "%d\n", height);
	fputs(str, outfile);
	/*write the maxval*/
	snprintf(str, 20, "%d\n", maxval);
	fputs(str, outfile);

	
	/*write the pixels*/
	for (int i = 0 ; i < height ; i++)
		fwrite(data[i], numcolors * width, 1, outfile);

	free(str);
	fclose(outfile);
}

void set_aux_matrix(int tasks, int width, int numcolors, unsigned char **source, unsigned char **aux)
{
	for(int i = 1 ; i <= tasks ; i++)
	{
		aux[i] = malloc(width * numcolors);
		aux[i][0] = source[i][0];
		aux[i][1] = source[i][1];
		aux[i][2] = source[i][2];
		aux[i][(width - 1) * numcolors] = source[i][(width - 1) * numcolors];
		aux[i][(width - 1) * numcolors + 1] = source[i][(width - 1) * numcolors + 1];
		aux[i][(width - 1) * numcolors + 2] = source[i][(width - 1) * numcolors + 2];
	}
}

void apply_filter(int tasks, int numcolors, int width, char *filter,
				 unsigned char **source_pixels, unsigned char **dest_pixels)
{
	for(int i = 1 ; i <= tasks ; i++)
	{
		for(int j = numcolors ; j < (width - 1) * numcolors ; j++)
		{
			if(strcmp(filter,"smooth") == 0)
				dest_pixels[i][j] = smoothing(i, j, source_pixels, numcolors);

			else if(strcmp(filter,"blur") == 0)
				dest_pixels[i][j] = gaussian(i, j, source_pixels, numcolors);

			else if(strcmp(filter,"sharpen") == 0)
				dest_pixels[i][j] = sharpen(i, j, source_pixels, numcolors);

			else if(strcmp(filter,"emboss") == 0)
				dest_pixels[i][j] = emboss(i, j, source_pixels, numcolors);

			else if(strcmp(filter,"mean") == 0)
				dest_pixels[i][j] = mean_removal(i, j, source_pixels, numcolors);
		}
	}
}

void free_buf (unsigned char **buf , int height)
{	
	if(buf != NULL)
	{	for(int i = 0 ; i < height ; i++)
			free(buf[i]);
		free(buf);
	}
}

int main(int argc, char* argv[])
{
	int rank;
	int nProcesses;
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &nProcesses);

	int numcolors;
	int width;
	int height;
	int maxval;
	int info[3];
	unsigned char **data;
	unsigned char **recvbuf;
	unsigned char **aux;
	int start;
	int tasks;
	
	if(rank == 0)
	{
		/*Read Input*/
		read_input(argv[1], &numcolors, &height, &width, &maxval, &data);

		/*Broadcast width, height, numcolors to all processes*/
		info[0] = width;
		info[1] = height;
		info[2] = numcolors;
		MPI_Bcast(&info, 3, MPI_INT, 0, MPI_COMM_WORLD);

		/*Send to each process its tasks*/
		/*tasks = number of lines that will be computed by a process*/
		tasks = height / nProcesses;
		if(height % nProcesses != 0)
			tasks++;

		for (int i = 1 ; i < nProcesses - 1 ; i++)
		{
			start = i * tasks - 1;
			for(int j = 0 ; j < tasks + 2 ; j++)
				MPI_Ssend(data[start + j], width * numcolors, MPI_CHAR, i, 0, MPI_COMM_WORLD);
		}
		/*Send last process what is left*/
		int leftTasks;
		if(nProcesses > 1)
		{
			start = (nProcesses - 1) * tasks - 1;
			leftTasks = height - tasks * (nProcesses - 1);
			for(int j = 0 ; j < leftTasks + 1 ; j++)
				MPI_Ssend(data[start + j], width * numcolors, MPI_CHAR, nProcesses-1, 0, MPI_COMM_WORLD);
		}

		/*Apply filters*/
		if(nProcesses == 1)
			tasks = tasks - 1;
		aux = malloc((tasks + 1) * sizeof(unsigned char *));
		set_aux_matrix(tasks - 1, width, numcolors, data, aux);


		for(int k = 3 ; k < argc ; k++)
		{
			apply_filter(tasks - 1, numcolors, width, argv[k], data, aux);
			for(int i = 1 ; i <= tasks - 1 ; i++)
			{
				memcpy(data[i], aux[i], width * numcolors);
			}

			/*If process 0 has a successor, exchange filtered lines needed to apply the next filter*/
			if(nProcesses > 1)
			{
				MPI_Ssend(data[tasks - 1], width * numcolors, MPI_CHAR, rank + 1, 0, MPI_COMM_WORLD);
				MPI_Recv(data[tasks], width * numcolors, MPI_CHAR, rank + 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			}
		}


		/*Receive processed pixels*/
		for (int i = 1 ; i < nProcesses - 1 ; i++)
		{
			start = i * tasks;
			for(int j = 0 ; j < tasks ; j++)
				MPI_Recv(data[start + j], width * numcolors, MPI_CHAR, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		}
		/*Receive from last process what is left*/
		if(nProcesses > 1)
		{
			start = (nProcesses - 1) * tasks;
			for(int j = 0 ; j < leftTasks ; j++)
				MPI_Recv(data[start + j], width * numcolors, MPI_CHAR, nProcesses-1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		}
			

		/*Write Output*/

		write_output(argv[2], numcolors, height, width, maxval, data);
		free_buf(aux, tasks + 1);
		free_buf(data, height);
		
	}
	else /*if it's not the first process*/
	{
		/*Receive width, height, numcolors*/
		MPI_Bcast(&info, 3, MPI_INT, 0, MPI_COMM_WORLD);
		width = info[0];
		height = info[1];
		numcolors = info[2];
		tasks = height / nProcesses;
		if (height % nProcesses != 0)
			tasks++;
			
		if(rank != nProcesses - 1)
		{

			/*Receive the lines that the process has to compute*/
			recvbuf = malloc((tasks + 2) * sizeof(unsigned char *));
			for(int j = 0 ; j < tasks + 2 ; j++)
			{ 
				recvbuf[j] = malloc(width * numcolors);
				MPI_Recv(recvbuf[j], width * numcolors, MPI_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			}
			/*Apply filter*/
			for(int k = 3 ; k < argc ; k++)
			{
				aux = malloc((tasks + 2) * sizeof(unsigned char *));
				aux[0] = malloc(width * numcolors);
				aux[tasks + 1] = malloc(width * numcolors);
				memcpy(aux[0], recvbuf[0], width * numcolors);
				memcpy(aux[tasks + 1], recvbuf[tasks + 1], width * numcolors);
				set_aux_matrix(tasks, width, numcolors, recvbuf, aux);
				apply_filter(tasks, numcolors, width, argv[k], recvbuf, aux);

				free_buf(recvbuf, tasks + 2);
				recvbuf = aux;

				/*Exchange information with predecessor and successor process in order to apply the next filter.*/
				MPI_Recv(recvbuf[0], width * numcolors, MPI_CHAR, rank - 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
				MPI_Ssend(recvbuf[1], width * numcolors, MPI_CHAR, rank - 1, 0, MPI_COMM_WORLD);
				MPI_Ssend(recvbuf[tasks], width * numcolors, MPI_CHAR, rank + 1, 0, MPI_COMM_WORLD);
				MPI_Recv(recvbuf[tasks + 1], width * numcolors, MPI_CHAR, rank + 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			}

			/*Send back processed lines*/
			for(int j = 0 ; j < tasks ; j++)
			{
				MPI_Ssend(recvbuf[j+1], width * numcolors, MPI_CHAR, 0, 0, MPI_COMM_WORLD);
			}
			free_buf(recvbuf, tasks + 2);
		}
		else
		/*if it's neither the first nor the last process*/
		{
			/*Receive the lines that the process has to compute*/
			tasks = height - tasks * (nProcesses - 1);
			recvbuf = malloc((tasks + 1) * sizeof(unsigned char *));
			for(int j = 0 ; j < tasks + 1 ; j++)
			{
				recvbuf[j] = malloc(width * numcolors);
				MPI_Recv(recvbuf[j], width * numcolors, MPI_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			}

			/*Apply filters*/
			for(int k = 3 ; k < argc ; k++)
			{
				aux = malloc((tasks + 1) * sizeof(unsigned char *));
				aux[0] = malloc(width * numcolors);
				aux[tasks] = malloc(width * numcolors);
				memcpy(aux[0], recvbuf[0], width * numcolors);
				memcpy(aux[tasks], recvbuf[tasks], width * numcolors);
				set_aux_matrix(tasks - 1, width, numcolors, recvbuf, aux);				
				apply_filter(tasks - 1, numcolors, width, argv[k], recvbuf, aux);

				free_buf(recvbuf, tasks + 1);
				recvbuf = aux;

				/*Exchange information with predecessor process in order to apply the next filter.*/
				MPI_Recv(recvbuf[0], width * numcolors, MPI_CHAR, rank - 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
				MPI_Ssend(recvbuf[1], width * numcolors, MPI_CHAR, rank - 1, 0, MPI_COMM_WORLD);
			}
			/*Send back processed lines*/
			for(int j = 0 ; j < tasks ; j++)
			{
				MPI_Ssend(recvbuf[j+1], width * numcolors, MPI_CHAR, 0, 0, MPI_COMM_WORLD);
			}
			free_buf(recvbuf, tasks + 1);

		}

	}

	MPI_Finalize();
	return 0;
	
}
