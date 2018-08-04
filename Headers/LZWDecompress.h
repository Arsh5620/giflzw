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
	/*
	We only need to have __declspec in MSVC++ as in gcc we compile with -fPIC and -shared
	*/
#if defined(__GCC__)
	//We have nothing to do in here but we still have it for completeness.
#elif defined(_MSC_VER)
#ifdef EXPORT
	__declspec(dllexport)
#else
	__declspec(dllimport)
#endif
#endif
LZWDecompress:
private LZWBase
{
private:
	//char *decompression_read_buffer; //Small 4 byte buffer to convert integer from big-to-little endian

	char *decompression_buffer; //Actual buffer where all the decompressed data will be stored
	int decompression_buffer_size = BUFFER_SIZE
		, decompression_buffer_pointer = 0; //The size and pointer for the decompression_buffer

	MyList<storage_info> *dictionary;

	unsigned char *compressed_data_buffer;
	int compressed_data_size = BUFFER_SIZE,
		compressed_data_pointer = 0;

	char bit_pointer = 0
		, byte_width = 0
		, default_byte_width = 0;
	bool can_push = false;

	::FILE *file_in;

	int last = -1;
	char lastchar = 0;
	char get_first_byte(storage_info *info);
	int read_next_bits();
	void read_compressed_stream(uint i_);
	void add_to_buffer(int code);
public:
#ifdef FILE_READ_BUILD
	LZWDecompress(std::FILE *file, int start_width = DEFAULT_BYTE_LEN);
#else
	LZWDecompress(unsigned char * memory, int buffer_size, int start_width = DEFAULT_BYTE_LEN);
#endif
	~LZWDecompress();
	void decompress();
	char *acquire_buffer(int *size);
};

