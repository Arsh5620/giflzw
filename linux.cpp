/*
This file does not use any licence file, but the author disclaims any interest in this file. This file is distributed without any WARRANTY written or implied, even the implied warranty of MERCHANTIBILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#include <stdio.h>
#include "./Headers/LZWCompress.h"
#include "./Headers/LZWDecompress.h"

/*
Returns the size of the file given the file pointer.
*/
int tell_size(::FILE *file){
	::fseek(file,0,SEEK_END);
	int read_size=(int)::ftell(file);
	::fseek(file,0,SEEK_SET);
	return read_size;
}

/*
This routine uses the LZWCompress class to compress the data.lzw file
*/
void compress(){
	printf("compress uses \"data.lzw\" file as input\n");

	::FILE *file=::fopen("data.lzw","rb");
	if(file==nullptr)
	{
		printf("cannot find the file \"data.lzw\"\n");
		exit(-1);
	}

	int file_size=tell_size(file);

	char *uncompressed=(char*)::malloc(file_size);
	if(::fread(uncompressed,1,file_size,file)==0){
		printf("could not read the file, an unknown error occured\n");
		exit(-1);
	}

	::fclose(file); 

	LZWCompress compress((uchar*)uncompressed,file_size);
	compress.compress();
	::free(uncompressed);

	int size=0;
	unsigned char *compressed=compress.acquire_buffer(&size);

	printf("compressed size is %d\n",size);

	file=::fopen("cm.lzw","wb");
	if(::fwrite(compressed, 1, size, file)){
		printf("compression completed\n");
	}

	::fclose(file);
	::free(compressed);
}

void decompress(){
	printf("decompression uses \"cm.lzw\" as input file\n");

	::FILE *file=::fopen("cm.lzw","rb");
	if(file==nullptr){
		printf("\"cm.lzw\" file is not found\"\n");
		exit(-1);
	}

	int file_size=tell_size(file);

	unsigned char *compressed=(unsigned char*)::malloc(file_size);
	if(::fread(compressed, 1, file_size, file)==0){
		printf("could not read the file, an unknown error occured\n");
		exit(-1);
	}

	file=::fopen("dc.lzw","wb");
	LZWDecompress decompress(compressed, file_size);
	decompress.decompress();

	::free(compressed);

	char *decompressed=decompress.acquire_buffer(&file_size);
	printf("decompressed size is %d\n",file_size);
	if(::fwrite(decompressed,1,file_size,file)){
		printf("decompression completed\n");
	}

	::free(decompressed);
	::fclose(file);
}
int main(int argc, char *argv[]){
	const char *help="Usage: ./<program-name> -[c|d|b]\n-c : compress the file\n-d : decompress the file\n-b : do compress and decompress both\n";
	if(argc!=2)
	{
		puts(help);
		exit(-1);
	}
	if(*argv[1]=='-'
		&& ::strlen(argv[1]) == 2){
		switch(argv[1][1])
		{
		case 'd':
		{
			decompress();
			break;
		}
		case 'c':
		{
			compress();
			break;
		}
		case 'b':
		{
			compress();
			decompress();
			break;
		}
		case 'h':
		{
			puts(help);
			break;
		}
		default:
			printf("Not a valid argument.\nUse ./<program-name> -h for help.\n");
		}
	}
	else
	{
		printf("Incorrect syntax for arguments.\nValid arguments are -c, -d ,-b.\nUse -h for help.\n");
	}
}
