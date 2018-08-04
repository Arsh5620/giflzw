/*
    GIFLZWLib --
    This library provides LZW compression and decompression routine for GIF stream compression.

    Author: Arshdeep Singh, copyleft 2017.
    LZW algorithm was originally created by Abraham Lempel, Jacob Ziv, and Terry Welch.
    
    Please see "LICENCE" to read the GPL.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#pragma once
#include "MyList.h"
#include "HashTable.h"
#include <memory.h>

#ifndef __GCC__
#include <fstream>
#endif

//Please note that the comments always precede the variables or methods they support, there can be stray comments too, if the comment is not directly followed by
//a variable or method in next line, it is highly probable its a stray comment

/*
The higher the number the higher the amount of values or keys, the dictionary can support
Also the dictionary size is related with the formula 1<<MAX_BYTE_LEN integer, example 12 as MAX_BYTE_LEN support dictionary with size of 4096
The dictionary memory footprint can be calculated with the formula, sizeof(storage_info) which is 12 * size of dictionary
So the memory footprint then is [[12*(1<<MAX_BYTE_LEN)]] approx.
The higher the size of items dictionary can support, the lower the dictionar purges and high compression which also slows down processing.
*/
#define MAX_BYTE_LEN 32 

/*
DEFAULT_BYTE_LEN is important variable(or it used to be in previous versions), as it defines the size of the input/output buffer's per code data.
Lower values such as 1 allows one code to have only 2 values, while higher values such as 8 are common for ASCII codes, lower values are only used in binary data
such as GIF when the palette size is low. A regular user using LZW for compression(which he must not use) should not change this value
*/
#define DEFAULT_BYTE_LEN 8 

/*
Deprecated CLEAR_CODE automatically generated in later builds.
Clear code is based on default_byte_len which was made a variable in latest builds so having clear_code as constant does not make any sense
same for end_of_information.
--------------------------------------------------
#define CLEAR_CODE (1<<DEFAULT_BYTE_LEN)
#define END_OF_INFORMATION (1<<DEFAULT_BYTE_LEN)+1
*/

/*
The comment below is intentional, uncomment the line below to build the library to support compression reading directly from the file

How does direct from file compression/decompression work?
We maintain a 4K/8K size buffer which we reload everytime we are <<about>> to finish the buffer, and we keep filling the buffer and (de/)compressing 
it until we are done with the whole file and the output stream is always written to memory regardless of the build type.

So what is the benefit?
You save a lot of memory that would otherwise be used to load up the file into memory buffer
, by replacing the memory with a constant size buffer that is filled over again, at the cost of negligible speed loss (2-4%, while not tested well).
*/

//#define FILE_READ_BUILD

/*
The buff_size is only used when building with file_read_build and is ignored in other builds,
It is the size of file read done when our in memory buffer is about to finish, so that it must be replaced with fresh buffer from disk
It helps to have set this value equal to your hard drive page size default 4096/8092
*/
#define BUFFER_SIZE 4096

/*
Most of the buffer size increases are done by shifting the current buffer sizes by a constant value, most likely 1(one)
Thus essentially multiplying by two, this is <<supposed>> to be faster operation to perform than a multiplication.
*/
#define BUFFER_GROW_SIZE 1

/*
Type: Structure
Explanation: This structure is used as the backbone for the MyList structure, as the MyList is just a huge array<<kind of>> of this structure
---------------
char_code: The ASCII code of the current character. | Say we have string "ABC", and we are processing "A", so char_code is set to 'A'.
_preval: The previous structure location in the array that this structure is an <<append>> to. 
		----------------------
		Well, what you just read might not be clear enough. here is an example, I have string "ABCD", and "ABCE" in memory how would I store it?
		Novice(as I was back then) I would store them as a two ascii strings, and that is it. but imagine thousands of such strings.
		Why waste memory in what itself is a <<compression>> software, why not use a data structure in which we only store back references.

		-----------       -----------       -----------
		|char_code|	  |char_code| 	    |char_code|
		|_preval  |<----  |_preval  |<----  |_preval  |
		|size_s	  |	  |size_s   |	    |size_s   |
		-----------	  -----------       -----------

		You might say why back references, because in the decoder stream we only store the code for the last structure's location so having a forward reference
		does not help in any way. having back reference with a size attribute helps.

		How do we write it to output stream, we move our pointer for output decompression buffer forward by size, and then we start walking back references
		and we keep writing char_code to our output buffer for each back reference we walk.
		----------------------
size_s: for each string we have a size of the string, we store the same property in the structure so we know how many structures come together to form this one string.
		or how many structures we need to walk to generate our string back.
---------------
Why not use classic strings, won't they be faster?
Ans:In general they are actually pretty fast, but the overhead of moving them around, and allocating memory for them because they are mutable and does not have a
	predefined size is a huge processing overhead, so we need to use this structure for faster optimizations.
*/
typedef struct {
	char char_code;
	int previous_code;
	int size;
} storage_info;

typedef unsigned int uint;
typedef unsigned char uchar;
typedef unsigned short ushort;

/*
EXPORT: is defined in the preprocessor settings for only GIFLZWLib, so if we try to include it in other project(s) it builds as __declspec(dllimport)
*/
class
#if defined(_MSC_VER)
#ifdef EXPORT
	__declspec(dllexport)
#else
	__declspec(dllimport)
#endif
#endif
	LZWBase
{
	int byte_default_start = 0;
public:
	LZWBase(int start_width = DEFAULT_BYTE_LEN);
	void *extend_buffer(void *, int, int);
	void check_to_extend(char **, int *, int);
	int  read_into_buffer(unsigned char *, int, ::FILE *);
	void push_default_elements(MyList<storage_info> *);
	void push_default_elements(HashTable *);
	bool step_byte_width(MyList<storage_info> *, char *);
	bool step_byte_width(HashTable * table, uchar * byte_width);
	void manual_hash_clean(HashTable * table, uchar * byte_width);
};

