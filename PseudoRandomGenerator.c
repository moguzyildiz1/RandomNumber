#include "randomgen.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

/*File descriptors initialized for spc_rand(), spc_entropy() etc.*/
static int spc_devrand_fd=-1;
static int spc_devrand_fd_noblock=-1;
static int devurand_fd=-1;


/*
	Sometimes the return value couldn't be equal to number of bytes you requested 
	because some implementations may limit the the amount of data you can read at once.
	When get a short read, justto read until collect enough data. 
	Or make the file descriptor non-blocking as below...
	In this case the function will return an error and set errno-> EAGAIN if there isn't
	enough data to complete the entire read. But sometimes the requested data could be 
	ready, then it will be returned. And return value of read() will be smaller than
	the request amount.

	Given an integer file descriptor, the following function makes
	the associated with the descriptor. 
*/
void spc_make_fd_nonblocking(int fd){

	int flags;
	
	flags=fcntl(fd,F_GETFL); //Get flags associated with the descriptor
	if(flags==-1){
		perror(spc_make_fd_nonblocking failed on F_GETFL);
		exit(-1);
	}
	flags|=O_NONBLOCK; //The flags will be the same as before, except with O_NONB. set
	if(fcntl(fd,F_SETFL,flags)==-1){
		perror("spc_make_fd_nonblocking failed on F_SETFL");
		exit(-1);
	}
}

/*
	Open /dev/random and /dev/urandom with open() function.
	Need to open /dev/random on two file descriptors, one blocking and the other
	is not, so that wemay avoid race conditions where spc_keygen()expects a 
	function to be non blocking but spc_entropy() has set the descriptorto blocking in
	other thread.
*/
void spc_rand_init(){
	spc_devrand_fd=open("/dev/random", O_RDONLY);
	spc_devrand_fd_noblock=open("/dev/random", O_RDONLY);
	spc_devurand_fd=open("/dev/urandom", O_RDONLY);

	if(spc_devrand_fd==-1 && spc_devrand_fd_noblock==-1){
		perror("spc_rand_init() failed to open /dev/random.");
		exit(-1);
	}
	if(spc_devurand_fd==-1){
		perror("spc_rand_init() failed to open /dev/urandom.");
		exit(-1);
	}
	//send non_blocking file descriptor to make_nonblock()
	spc_make_fd_nonblocking(spc_devrand_fd_noblock);
}//end of spc_rand_init()

/*
	
*/
unsigned char *spc_rand(unsigned char *buf, size_t nbytes){
	ssize_t r;
	unsigned char *where=buf;

	if(spc_devrand_fd==-1 && spc_devrand_fd_noblock==-1 && spc_devurand_fd==-1){
		spc_rand_init();
	}
	while(nbytes){
		if((r=read(spc_devurand_fd,where,nbytes))==-1){
			if(errno==EINTR) continue;
			perror("spc_rand couldn't read from /dev/urandom.");
			exit(-1);
		}
		where+=r;
		nbytes-=r;
	}
	return buf;
}

/*
	Seeds /dev/random and /dev/urandom to generate random numbers
*/
unsigned char *spc_keygen(unsigned char *buf, size_t nbytes){
	ssize_t r;
	unsigned char *where=buf;

	if(spc_devrand_fd==-1 && spc_devrand_fd_noblock==-1 && spc_devurand_fd==-1){
		spc_rand_init();
	}
	while(nbytes){
		if((r=read(spc_devrand_fd_noblock,where,nbytes))==-1){
			if(errno==EINTR) continue;
			if(errno==EAGAIN) break;
			perror("spc_rand couldn't read from /dev/random.");
			exit(-1);
		}
		where+=r;
		nbytes-=r;
	}
	spc_rand(where,nbytes);
	return buf;
}

unsigned char *spc_entropy(unsigned char *buf, size_t nbytes){
	ssize_t r;
	unsigned char *where=buf;

	if(spc_devrand_fd==-1 && spc_devrand_fd_noblock==-1 && spc_devurand_fd==-1){
		spc_rand_init();
	}
	while(nbytes){
		if((r=read(spc_devrand_fd,(void*)where,nbytes))==-1){
			if(errno==EINTR) continue;
			perror("spc_rand couldn't read from /dev/random.");
			exit(-1);
		}
		where+=r;
		nbytes-=r;
	}
	return buf;
}