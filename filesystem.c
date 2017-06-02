#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include "libdisksimul.h"
#include "filesystem.h"

int getDirSectorAdress( char* dirPath )
{
	struct root_table_directory root_dir;
	struct table_directory dirTable; 
	char* subDir;
	char* aux;
	char dirName[20];
	int i, flag, dirTableAdress;
	
	ds_read_sector(0, (void*)&root_dir, SECTOR_SIZE);
	
	if( dirPath[0] != '/' )
	{
		printf("Invalid directory path, not an abolsute path\n" );
		return -1;
	}
	
	subDir = strchr( dirPath+1, '/' );
	
	if( subDir == NULL )
	{
		return 0; //root directory
	}
		
	memset( dirName, 0, 20 );
	
	aux = dirPath+1;
	
	for( i = 0; aux != subDir; i++ )
	{
		dirName[i] = *aux;
		aux++;
	}
	
	flag = 0;
	
	printf("Dir name: %s\n", dirName );
	
	for( i = 0; i < 15; i++ )
	{
		if( (strcmp( dirName, root_dir.entries[i].name ) == 0) && (root_dir.entries[i].dir == 1 ) ) //mesmo nome
		{
			dirTableAdress = root_dir.entries[i].sector_start;
			ds_read_sector( dirTableAdress, (void*)&dirTable, SECTOR_SIZE );
			flag = 1;
			break;
		}
	}	
	
	if( flag == 0 )
	{
		printf("Directory not found\n");
		return -1;
	}
	
	subDir = strchr( subDir+1, '/' );
	
	while( subDir != NULL )
	{
		aux++;
		flag = 0;
		memset( dirName, 0, 20 );
		//aux = subDir+1;
		
		for( i = 0; aux != subDir; i++ )
		{
			dirName[i] = *aux;
			aux++;
		}
		
		printf("Dir name: %s\n", dirName );
		
		for( i = 0; i < 16; i++ )
		{
			if( ( strcmp( dirName, dirTable.entries[i].name ) == 0 ) && ( dirTable.entries[i].dir == 1 ) ) //ver a questao de arquivo x diretorio
			{
				dirTableAdress = dirTable.entries[i].sector_start;
				ds_read_sector( dirTableAdress, (void*)&dirTable, SECTOR_SIZE );
				flag = 1;
				break;
			}
		}
		
		if( flag == 0 )
		{
			printf("Directory not found\n");
			return -1;
		}
		
		subDir = strchr( subDir+1, '/' );
	}
	
	return dirTableAdress;
	
}


/**
 * @brief Format disk.
 * 
 */
int fs_format(){
	int ret, i;
	struct root_table_directory root_dir;
	struct sector_data sector;
	
	if ( (ret = ds_init(FILENAME, SECTOR_SIZE, NUMBER_OF_SECTORS, 1)) != 0 ){
		return ret;
	}
	
	memset(&root_dir, 0, sizeof(root_dir));
	
	root_dir.free_sectors_list = 1; /* first free sector. */
	
	ds_write_sector(0, (void*)&root_dir, SECTOR_SIZE);
	
	/* Create a list of free sectors. */
	memset(&sector, 0, sizeof(sector));
	
	for(i=1;i<NUMBER_OF_SECTORS;i++){
		if(i<NUMBER_OF_SECTORS-1){
			sector.next_sector = i+1;
		}else{
			sector.next_sector = 0;
		}
		ds_write_sector(i, (void*)&sector, SECTOR_SIZE);
	}
	
	ds_stop();
	
	printf("Disk size %d kbytes, %d sectors.\n", (SECTOR_SIZE*NUMBER_OF_SECTORS)/1024, NUMBER_OF_SECTORS);
	
	return 0;
}

/**
 * @brief Create a new file on the simulated filesystem.
 * @param input_file Source file path.
 * @param simul_file Destination file path on the simulated file system.
 * @return 0 on success.
 */
int fs_create(char* input_file, char* simul_file){
	int ret, i, flag;
	FILE *inputFd = NULL;
	struct file_dir_entry newFile;
	struct sector_data tmpSector;
	struct root_table_directory root_dir;
	struct table_directory dirTable;	
	int dirAdress;
	char * newFileName;
	
	printf("Creating file\n" );
		
		
	if ( (ret = ds_init(FILENAME, SECTOR_SIZE, NUMBER_OF_SECTORS, 0)) != 0 ){
		printf("asds\n" );
		return ret;
	}

	/* Write the code to load a new file to the simulated filesystem. */
	
	inputFd = fopen( input_file, "r+b" );

	if( inputFd == NULL )
	{
		printf( "Input file not found\n");
		ds_stop();
		return -1;
	}

	newFileName = strrchr( simul_file, '/' ); //aponta pro ultimo '/'
	newFileName = newFileName + 1;
	
	if( newFileName[0] == '\0' ) // se '/' for o ultimo caractere
	{
		printf("Invalid file source, no file name specified \n" );
		ds_stop();
		return -1;
	}
	
	fseek( inputFd, 0, SEEK_END ); //0 or 0L
	int inputSize = ftell( inputFd );
	printf("inputFile size: %d\n", inputSize );
	
	newFile.dir = 0;
	strcpy( newFile.name, newFileName );
	newFile.size_bytes = inputSize;


	ds_read_sector(0, (void*)&root_dir, SECTOR_SIZE);
	newFile.sector_start = root_dir.free_sectors_list;
	
	
	dirAdress = getDirSectorAdress( simul_file );
	if( dirAdress < 0 )
	{
		ds_stop();
		return -1;
	}
		
	if( dirAdress == 0 ) // it is trying to write a file in the root dir
	{
		flag = 0;
		
		for( i = 0; i < 15; i++ )
		{
			
			if( ( root_dir.entries[i].dir == 0 ) && ( strcmp( root_dir.entries[i].name, newFileName ) == 0 ) ) //se ja existe file
			{
				printf("There already is a file called '%s'. Exiting.\n", newFileName );
				ds_stop();
				return -1;
			}
		}
		
		for( i = 0; i < 15; i++ )
		{
			
			if( ( root_dir.entries[i].dir == 0 ) && ( root_dir.entries[i].size_bytes == 0 ) ) //se for vazio
			{
				root_dir.entries[i] = newFile;
				flag = 1;
				break;
			}
		}
		
		if( flag == 0 )
		{
			printf("The destination path ( root directory ) is full!\n" );
			ds_stop();
			return -1;
		}
		
		//root_dir updating happens in the end
	}
	
	else
	{
		ds_read_sector( dirAdress, (void*)&dirTable, SECTOR_SIZE );
		
		for( i = 0; i < 16; i++ )
		{
			
			if( ( dirTable.entries[i].dir == 0 ) && ( strcmp( dirTable.entries[i].name, newFileName ) == 0 ) ) //se ja existe file
			{
				printf("There already is a file called '%s'. Exiting.\n", newFileName );
				ds_stop();
				return -1;
			}
		}
		
		flag = 0;
		for( i = 0; i < 16; i++ )
		{
			if( ( dirTable.entries[i].dir == 0 ) && ( dirTable.entries[i].size_bytes == 0 ) ) //se for vazio
			{
				dirTable.entries[i] = newFile;
				flag = 1;
				break;
			}
		}
		
		if( flag == 0 )
		{
			printf("The destination path in the virtual disk is full!\n" );
			ds_stop();
			return -1;
		}
		
		ds_write_sector( dirAdress, (void*)&dirTable, SECTOR_SIZE ); // saves the updated directory		
	}
	

	int rest = inputSize % DATA_SIZE; // what rest of the last block
	int numOfSectors; //number of sectors used - 1
	int lastBlockSize;
	if( rest > 0 ) //is not multiple
	{
		lastBlockSize = rest;
		numOfSectors = ( inputSize / DATA_SIZE ) + 1;
	}
	else
	{
		lastBlockSize = DATA_SIZE;
		numOfSectors = inputSize / DATA_SIZE;
	}
	
	int sectorAdress = newFile.sector_start;
	char data[DATA_SIZE];
	
	memset( data, 0, DATA_SIZE );

	for( i = 0; i < numOfSectors - 1; i++ )
	{
		ds_read_sector( sectorAdress, (void*)&tmpSector, SECTOR_SIZE );
		fread( data, sizeof(char), DATA_SIZE, inputFd ); 
		ds_write_sector( sectorAdress, (void*)&data, DATA_SIZE ); 
		
		sectorAdress = tmpSector.next_sector;
	}

	
	ds_read_sector( sectorAdress, (void*)&tmpSector, SECTOR_SIZE );
	
	root_dir.free_sectors_list = tmpSector.next_sector;
	ds_write_sector( 0, (void*)&root_dir, SECTOR_SIZE );
	
	memset( &tmpSector, 0, SECTOR_SIZE );
	fread( tmpSector.data, sizeof(char), lastBlockSize, inputFd );
	ds_write_sector( sectorAdress, (void*)&tmpSector, SECTOR_SIZE ); 

	//pegar o tamanho de inputFd e colocar no tamanho do newFIle
	//pegar o nome de inputFd e colocar no nome do newFile
	//colocar o proximo bloco livre( q vai ter no root_table_directory ) como o start do newFile
	//escrever dados no newFile, em blocos, com o que tem no inputFd
	
	
	ds_stop();
	
	return 0;
}

/**
 * @brief Read file from the simulated filesystem.
 * @param output_file Output file path.
 * @param simul_file Source file path from the simulated file system.
 * @return 0 on success.
 */
int fs_read(char* output_file, char* simul_file){
	int ret;
	if ( (ret = ds_init(FILENAME, SECTOR_SIZE, NUMBER_OF_SECTORS, 0)) != 0 ){
		return ret;
	}
	
	/* Write the code to read a file from the simulated filesystem. */
	
	ds_stop();
	
	return 0;
}

/**
 * @brief Delete file from file system.
 * @param simul_file Source file path.
 * @return 0 on success.
 */
int fs_del(char* simul_file){
	int ret;
	if ( (ret = ds_init(FILENAME, SECTOR_SIZE, NUMBER_OF_SECTORS, 0)) != 0 ){
		return ret;
	}
	
	/* Write the code delete a file from the simulated filesystem. */
	
	ds_stop();
	
	return 0;
}

/**
 * @brief List files from a directory.
 * @param simul_file Source file path.
 * @return 0 on success.
 */
int fs_ls(char *dir_path){
	int ret, i;
	if ( (ret = ds_init(FILENAME, SECTOR_SIZE, NUMBER_OF_SECTORS, 0)) != 0 ){
		return ret;
	}
	
	/* Write the code to show files or directories. */
	
	struct root_table_directory root_dir;
	struct table_directory dirTable;
	int dirAdress = getDirSectorAdress( dir_path );
	
	if( dirAdress < 0 )
	{
		ds_stop();
		return -1;
	}
		
	printf( "Listing '%s' directory:\n", dir_path );
	
	
	if( dirAdress == 0 )
	{
		ds_read_sector( 0, (void*)&root_dir, SECTOR_SIZE );
		
		for( i = 0; i < 15; i++ )
		{
			if( (root_dir.entries[i].dir == 1) )
			{
				printf( "\t- [d] %s\n", root_dir.entries[i].name );
			}
			else if( (root_dir.entries[i].dir == 0) && (root_dir.entries[i].size_bytes > 0) )
			{
				printf( "\t- [f] %s\t%d Bytes\n", root_dir.entries[i].name, root_dir.entries[i].size_bytes );
			}
		}
	}
	
	else
	{
		ds_read_sector( 0, (void*)&dirTable, SECTOR_SIZE );
		
		for( i = 0; i < 16; i++ )
		{
			if( (dirTable.entries[i].dir == 1) )
			{
				printf( "\t- [d] %s\n", dirTable.entries[i].name );
			}
			else if( (dirTable.entries[i].dir == 0) && (dirTable.entries[i].size_bytes > 0) )
			{
				printf( "\t- [f] %s\t%d Bytes\n", dirTable.entries[i].name, dirTable.entries[i].size_bytes );
			}
		}
	}
	
	printf("\n" );
	ds_stop();
	
	return 0;
}

/**
 * @brief Create a new directory on the simulated filesystem.
 * @param directory_path directory path.
 * @return 0 on success.
 */
int fs_mkdir(char* directory_path){
	int ret;
	if ( (ret = ds_init(FILENAME, SECTOR_SIZE, NUMBER_OF_SECTORS, 0)) != 0 ){
		return ret;
	}
	
	/* Write the code to create a new directory. */
	
	ds_stop();
	
	return 0;
}

/**
 * @brief Remove directory from the simulated filesystem.
 * @param directory_path directory path.
 * @return 0 on success.
 */
int fs_rmdir(char *directory_path){
	int ret;
	if ( (ret = ds_init(FILENAME, SECTOR_SIZE, NUMBER_OF_SECTORS, 0)) != 0 ){
		return ret;
	}
	
	/* Write the code to delete a directory. */
	
	ds_stop();
	
	return 0;
}

/**
 * @brief Generate a map of used/available sectors. 
 * @param log_f Log file with the sector map.
 * @return 0 on success.
 */
int fs_free_map(char *log_f){
	int ret, i, next;
	struct root_table_directory root_dir;
	struct sector_data sector;
	char *sector_array;
	FILE* log;
	int pid, status;
	int free_space = 0;
	char* exec_params[] = {"gnuplot", "sector_map.gnuplot" , NULL};

	if ( (ret = ds_init(FILENAME, SECTOR_SIZE, NUMBER_OF_SECTORS, 0)) != 0 ){
		return ret;
	}
	
	/* each byte represents a sector. */
	sector_array = (char*)malloc(NUMBER_OF_SECTORS);

	/* set 0 to all sectors. Zero means that the sector is used. */
	memset(sector_array, 0, NUMBER_OF_SECTORS);
	
	/* Read the root dir to get the free blocks list. */
	ds_read_sector(0, (void*)&root_dir, SECTOR_SIZE);
	
	next = root_dir.free_sectors_list;

	while(next){
		/* The sector is in the free list, mark with 1. */
		sector_array[next] = 1;
		
		/* move to the next free sector. */
		ds_read_sector(next, (void*)&sector, SECTOR_SIZE);
		
		next = sector.next_sector;
		
		free_space += SECTOR_SIZE;
	}

	/* Create a log file. */
	if( (log = fopen(log_f, "w")) == NULL){
		perror("fopen()");
		free(sector_array);
		ds_stop();
		return 1;
	}
	
	/* Write the the sector map to the log file. */
	for(i=0;i<NUMBER_OF_SECTORS;i++){
		if(i%32==0) fprintf(log, "%s", "\n");
		fprintf(log, " %d", sector_array[i]);
	}
	
	fclose(log);
	
	/* Execute gnuplot to generate the sector's free map. */
	pid = fork();
	if(pid==0){
		execvp("gnuplot", exec_params);
	}
	
	wait(&status);
	
	free(sector_array);
	
	ds_stop();
	
	printf("Free space %d kbytes.\n", free_space/1024);
	
	return 0;
}

