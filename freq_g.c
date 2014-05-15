/*
 *	GPCLK0 Frequency Controller by blog.everpi.net
 *
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <errno.h>

#define MAPSIZE 0x01000000
#define GPF_SEL0 0x200000
#define CM_GP0CTL 0x101070
#define CM_GP0DIV 0x101074

#define CM_GP1CTL 0x101078
#define CM_GP1DIV 0x10107c

#define HELP \
        "\n  \033[1mGPCLK0 Frequency Controller by blog.everpi.net\n\n" \
        "\tUsage: freq_g [frequency] [source]\n\n" \
        "\t\t [frequency] in Mhz with or without float pointing\n" \
	"\t\t [source]    number is the frequency source argument\n\n" \
	"\tSupported sources: 1 - XTAL 19.2Mhz\n" \
 	"\t\t\t   5 - PLLC 1000Mhz\n" \
	"\t\t\t   6 - PLLD 500Mhz default\n" \
	"\t\t\t   7 - HDMI aux 216Mhz\033[0m\n\n"

#define _ERR(c) if(c == -1){ fprintf(stderr,"%s\n",strerror(errno)); return 1; }

int main(int argc, char *argv[]){
	
	volatile uint32_t *map = NULL;
	int memfd = 0;	
	int err   = 0;
	float sum = 0;	
	float clock = 0;	

	struct cm_gpctl{
	
		uint32_t src:      4; // Clock source
		uint32_t enab:     1; // Enable the clock generator
		uint32_t kill:     1; // Kill the clock generator
		uint32_t unused:   1; // Unused bit
		uint32_t busy:     1; // Indicates if the clock generator is running
		uint32_t flip:     1; // Invert the clock generator output
		uint32_t mash:     2; // MASH Control
		uint32_t unused2: 13; // Unused bits
		uint32_t passwd:   8; // Clock manager password "0x5a"
	
	}gpctl = { 6,   // src
		   1,   // enab
		   0,   // kill 
		   0,   // unused
		   0,   // busy
		   0,   // flip
		   0,   // mash
		   0,   // unused2
		   0x5a // passwd
	};

	if(argc < 2){
		printf("%s",HELP);
		return 0;
	}
	
	memfd = open("/dev/mem", O_RDWR|O_SYNC);
	_ERR(memfd);
	
	map = (unsigned int *)mmap(NULL,MAPSIZE,PROT_READ|PROT_WRITE,
				   MAP_SHARED,memfd,0x20000000);
	_ERR((int)map);	
	
	*(volatile uint32_t *)((uint32_t)map+GPF_SEL0 ) &= ~(7 << 12); // clear triple cfg of gpio4
	*(volatile uint32_t *)((uint32_t)map+GPF_SEL0 ) |=  (4 << 12); // set gpio4 to ALT0
	
	
	if(argc > 2){

		gpctl.src = strtol(argv[2],NULL,10);

		switch(gpctl.src){
			
			case 1: clock = 19.2; break;   // XTAL
			case 5: clock = 1000.0; break; // PLLC
			case 6: clock = 500.0; break;  // PLLD
			case 7: clock = 216.0; break;  // HDMI auxiliary
			
		}
	}else{
		clock = 500.0; // PLLD
		gpctl.src = 6;
	}
	
	*(volatile uint32_t *)((uint32_t)map+CM_GP0CTL) = *((int *)&gpctl);	

	printf("%p - %p\n",map,((uint32_t)map+0x101070));
	
	sum = strtod(argv[1],NULL);	
	printf("Frequency:%f\n", sum);
	printf("Normal divisor:%f\n",clock / sum);
	printf("Divisor:%d\n",(int)((clock / sum ) * (float)(1<<12) + 0.5));	
	*(volatile uint32_t *)((uint32_t)map+CM_GP0DIV) = (0x5a << 24) + 1<<12;
							 /*(int)( (clock / sum ) * 
					                 (float)(1<<12) + 0.5 );*/

	err = munmap((uint32_t *)map,MAPSIZE);	
	_ERR(err);
	
	close(memfd);
	_ERR(err);	

	return 0;
}
