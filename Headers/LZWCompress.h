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
#include "LZWBase.h"

class
#if defined(_MSC_VER)
#ifdef EXPORT
	__declspec(dllexport)
#else
	__declspec(dllimport)
#endif
#endif
LZWCompress: private LZWBase
{
private:
	/*
	Buffer where the compressed stream is output to.
	*/
	uchar *compression_buffer;

	/*
	The size of the above buffer, defaults to BUFFER_SIZE
	*/
	int compression_buffer_size = BUFFER_SIZE;

	/*
	The pointer to track location of the bufferat which we are going to write next values
	*/
	int compression_buffer_pointer = 0;

	/*
	bit_pointer is used to keep track of bit inside a byte as we write a value that overlaps byte boundary
	*/
	uchar bit_pointer = 0
		, byte_width = 0 // The width of the LZW codes that we will output to the compression_buffer
		, default_byte_width = 0; // The byte width that "byte_width" must be reset to once it is above MAX_BYTE_LEN

#ifdef FILE_READ_BUILD
	std::FILE *file_in; // The handle to the file that the library is compressing
	uchar buffer[BUFFER_SIZE]; // buffer is the actual data that we are compressing
	long file_size = 0; // Used to track the size of the file the program is processing
#else
	uchar *buffer;
#endif

	int buffer_size // used to track the size of the buffer, so that we can terminate the program when done
		, buffer_pointer = 0; // used to track the processing state of the buffer, which tells us about what next code to process

	/*
	table stores all the dictionary structures that form the basis of LZW compression.
	*/
	HashTable *table; 

	void write_multibyte_buffer(uint information);
public:
#ifdef FILE_READ_BUILD
	LZWCompress(std::FILE *, int start_width = DEFAULT_BYTE_LEN);
#else
	LZWCompress(uchar *, int buffer_size, int start_width = DEFAULT_BYTE_LEN);
#endif
	~LZWCompress();
	int compress();
	uchar *acquire_buffer(int *size);
};

