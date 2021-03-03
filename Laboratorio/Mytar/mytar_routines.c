#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "mytar.h"

extern char *use;

/** Copy nBytes bytes from the origin file to the destination file.
 *
 * origin: pointer to the FILE descriptor associated with the origin file
 * destination:  pointer to the FILE descriptor associated with the destination file
 * nBytes: number of bytes to copy
 *
 * Returns the number of bytes actually copied or -1 if an error occured.
 */
int
copynFile(FILE * origin, FILE * destination, int nBytes)
{
	char* bytes = malloc(nBytes);

	int oBytes = fread(bytes,1,nBytes,origin);
	int dBytes = fwrite(bytes,1,oBytes,destination);

	free(bytes);

	if (oBytes != dBytes) return -1;
	else return oBytes;
}

/** Loads a string from a file.
 *
 * file: pointer to the FILE descriptor 
 * 
 * The loadstr() function must allocate memory from the heap to store 
 * the contents of the string read from the FILE. 
 * Once the string has been properly built in memory, the function returns
 * the starting address of the string (pointer returned by malloc()) 
 * 
 * Returns: !=NULL if success, NULL if error
 */
char*
loadstr(FILE * file)
{
	int strSize = 0; char c;
	while ((c = getc(file)) != '\0' && c != EOF) strSize++;

	if (strSize == 0) return NULL;

	char *buf = malloc(sizeof(char)*strSize);
	fseek(file,-strSize-1,SEEK_CUR);
	fread(buf,sizeof(char),strSize+1,file);

	return buf;
}

/** Read tarball header and store it in memory.
 *
 * tarFile: pointer to the tarball's FILE descriptor 
 * nFiles: output parameter. Used to return the number 
 * of files stored in the tarball archive (first 4 bytes of the header).
 *
 * On success it returns the starting memory address of an array that stores 
 * the (name,size) pairs read from the tar file. Upon failure, the function returns NULL.
 */
stHeaderEntry*
readHeader(FILE * tarFile, int *nFiles)
{
	stHeaderEntry* array = NULL;
	int nr_files = 0;
	fread(&nr_files,sizeof(int),1,tarFile);

	// Reservamos memoria para el array
	array = malloc(sizeof(stHeaderEntry)*nr_files);

	for (int i = 0; i < nr_files; ++i){
		array[i].name = loadstr(tarFile);
		fread(&(array[i].size),sizeof(int),1,tarFile);
	}

	// Devolvemos los valores leidos a la funcion invocadora
	(*nFiles) = nr_files;
	
	return (array);
}

/** Creates a tarball archive 
 *
 * nfiles: number of files to be stored in the tarball
 * filenames: array with the path names of the files to be included in the tarball
 * tarname: name of the tarball archive
 * 
 * On success, it returns EXIT_SUCCESS; upon error it returns EXIT_FAILURE. 
 * (macros defined in stdlib.h).
 *
 * HINTS: First reserve room in the file to store the tarball header.
 * Move the file's position indicator to the data section (skip the header)
 * and dump the contents of the source files (one by one) in the tarball archive. 
 * At the same time, build the representation of the tarball header in memory.
 * Finally, rewind the file's position indicator, write the number of files as well as 
 * the (file name,file size) pairs in the tar archive.
 *
 * Important reminder: to calculate the room needed for the header, a simple sizeof 
 * of stHeaderEntry will not work. Bear in mind that, on disk, file names found in (name,size) 
 * pairs occupy strlen(name)+1 bytes.
 *
 */
int
createTar(int nFiles, char *fileNames[], char tarName[])
{
	// 1) Abrimos el fichero mtar para escritura (fichero destino)
	FILE* tarFile = fopen(tarName,"w");
	if (tarFile == NULL) return EXIT_FAILURE;

	// 2) Reservamos memoria (con malloc()) para un array de stHeaderEntry
	int headerSize = sizeof(int);
	for (int i = 0; i < nFiles; ++i){
		headerSize += strlen(fileNames[i]) + sizeof(int) + 1;
	}
	stHeaderEntry* array = malloc(headerSize);

	// 3) Saltamos al byte del fichero mtar donde comienza la region de datos
	fseek(tarFile,headerSize,SEEK_SET);

	// 4) Por cada (inputFile) que haya que copiar en el mtar
	for (int i = 0; i < nFiles; i++){
		FILE* inputFile = fopen(fileNames[i],"r");
		int copiedBytes = copynFile(inputFile,tarFile,8192); // 8192 max bytes leidos
		fclose(inputFile);

		array[i].name = malloc(strlen(fileNames[i])+1);
		strcpy(array[i].name,fileNames[i]);
		array[i].size = copiedBytes;
	}

	// 5) Nos posicionamos para escribir en el byte 0 del fichero mtar
	fseek(tarFile,0,SEEK_SET);
	fwrite(&nFiles,sizeof(int),1,tarFile);
	for (int i = 0; i < nFiles; ++i){
		fwrite(array[i].name,sizeof(char),strlen(array[i].name)+1,tarFile);
		fwrite(&array[i].size,sizeof(int),1,tarFile);
	}

	// 6) Liberamos memoria y cerramos el fichero mtar
	free(array);
	fclose(tarFile);

	return EXIT_SUCCESS;
}

/** Extract files stored in a tarball archive
 *
 * tarName: tarball's pathname
 *
 * On success, it returns EXIT_SUCCESS; upon error it returns EXIT_FAILURE. 
 * (macros defined in stdlib.h).
 *
 * HINTS: First load the tarball's header into memory.
 * After reading the header, the file position indicator will be located at the 
 * tarball's data section. By using information from the 
 * header --number of files and (file name, file size) pairs--, extract files 
 * stored in the data section of the tarball.
 *
 */
int
extractTar(char tarName[])
{
	FILE* tarFile = fopen(tarName,"r");
	if (tarFile == NULL) return EXIT_FAILURE;

	int nFiles;
	stHeaderEntry* array = readHeader(tarFile,&nFiles);
	for (int i = 0; i < nFiles; ++i){
		FILE* file = fopen(array[i].name,"w");
		copynFile(tarFile,file,array[i].size);
		fclose(file);
	}

	free(array);
	fclose(tarFile);

	return EXIT_SUCCESS;
}
