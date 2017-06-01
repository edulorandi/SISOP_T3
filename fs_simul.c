#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "filesystem.h"

void usage(char *exec){
	printf("%s -format\n", exec);
	printf("%s -create <disk file> <simulated file>\n", exec);
	printf("%s -read <disk file> <simulated file>\n", exec);
   printf("%s -ls <absolute directory path>\n", exec);
	printf("%s -del <simulated file>\n", exec);
	printf("%s -mkdir <absolute directory path>\n", exec);
	printf("%s -rmdir <absolute directory path>\n", exec);
}


int main(int argc, char **argv){
	
	if(argc<2)
	{
		usage(argv[0]);
	}
	
	else
	{
		/* Disk formating. */
		if( !strcmp(argv[1], "-format"))
		{
			fs_format();
		}
		
		else if( !strcmp(argv[1], "-create") )
		{
			if( argc < 3 )
			{
				usage( argv[0] );
			}
			
			else
			{
				printf("argv[2]: %s\n", argv[2] );
				printf("argv[3]: %s\n", argv[3] );
				
				fs_create( argv[2], argv[3] );
			}
		}
	}
	
	
	/* Implement the other filesystem calls here!! */
	
	
	
	/* Create a map of used/free disk sectors. */
	fs_free_map("log.dat");
	
	return 	0; 
}
	
