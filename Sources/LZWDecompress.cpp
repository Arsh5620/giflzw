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

#include "../Headers/LZWDecompress.h"

/*
Walks down the complete structure heirarchy to find the first storage_info struct that does not have any back reference
*/
char LZWDecompress::get_first_byte(storage_info * info)
{
	while (info->previous_code != -1)
		info = (*dictionary).operator[](info->previous_code);
	return info->char_code;
}

/*
This function read what is written by write_multibyte_buffer
*/
int LZWDecompress::read_next_bits()
{
	// Since LZW stream and both intel x86 is big endian the following is possible
	unsigned int i = *(uint*)(this->compressed_data_buffer + this->compressed_data_pointer);
	i = i >> (this->bit_pointer); // To clear extra bits while reading
	i = i & ((1 << (byte_width)) - 1); // Optimized from: "i=i<<shift;i=i>>shift;"

	this->bit_pointer += byte_width;
	if (this->bit_pointer >= this->default_byte_width) {
		(this->compressed_data_pointer) += (this->bit_pointer >> 3);
		this->bit_pointer %= this->default_byte_width;
	}
	return(i);
}

/*
This function actually does the decompression, and puts the decompressed text to output, and add it to dictionary too.
*/
void LZWDecompress::read_compressed_stream(uint index_code)
{
	/*
	Why do we have can_push, because we cannot have the function adding values into dictionary that already exist.
	*/
	if (can_push) {
		storage_info *node = (*dictionary)[((index_code >= dictionary->list_size()) ? last : index_code)];

		storage_info info;
		info.char_code = get_first_byte(node);
		info.previous_code = last;
		info.size = (*dictionary).operator[](last)->size + 1;
		(*dictionary).push(info);
	}
	else {
		can_push = true;
		last = -1;
	}

	if (index_code < 256) {
		// If index_code is a basic code (is in ASCII), just write it at one byte
		lastchar = (char)index_code;
	}
	else {
		/*
		Else copy that code over and set the pointer to the length of such code
		*/
		lastchar = (*dictionary)[index_code]->char_code;
	}

	//Now write the whole sequence string to the output buffer(decompression_buffer)
	add_to_buffer(index_code);

	//Set the last code found
	last = index_code;
}

/*
This functions appends a string to the decompressed_buffer(or any other buffer)
*/
void LZWDecompress::add_to_buffer(int code)
{
	storage_info *info = (*dictionary).operator[](code);
	int size = info->size
		, update_size = this->decompression_buffer_size;
	bool buffer_grew = false;

	/*
	Loop through while checking if update_size*2 would be enough to store the new string, we need to make sure we have enough memory to store the new string
	*/
	while (update_size < (this->decompression_buffer_pointer + info->size)) {
		update_size <<= BUFFER_GROW_SIZE;
		buffer_grew = true;
	}

	// If we would have to expand/increase the current buffer, we would have to call appropriate methods to do so
	if (buffer_grew) {
		this->decompression_buffer = (char*)LZWBase::extend_buffer(this->decompression_buffer, this->decompression_buffer_size, update_size);
		this->decompression_buffer_size = update_size;
	}

	// In optimized code we increase performance by reducing memory operations such as move and copy.
	char *buffer_copy_begin = (this->decompression_buffer + this->decompression_buffer_pointer);
	char *buffer_copy_end = buffer_copy_begin + (info->size - 1);

	//The last character in the newly copied string would the character at the current "code" in the list dictionary.
	*buffer_copy_end = info->char_code;
	--buffer_copy_end;

	// If the code we have to copy is more than one byte
	if (code) {
		while (info->previous_code != -1
			&& buffer_copy_end >= buffer_copy_begin) {
			info = (*dictionary).operator[](info->previous_code);
			*buffer_copy_end = info->char_code;
			--buffer_copy_end;
		}
	}
	this->decompression_buffer_pointer += size;
}

/*
Constructor: Initializes list structure and fills with default values.
*/
LZWDecompress::LZWDecompress
#ifdef FILE_READ_BUILD
(std::FILE *file, int start_width)
#else
(unsigned char *memory, int buffer_size, int start_width)
#endif
	:LZWBase(start_width) 
{
	this->default_byte_width =
		this->byte_width = (char)start_width;

#ifdef FILE_READ_BUILD
	file_in = file;

	compressed_data_buffer = (unsigned char*)::malloc(BUFFER_SIZE);
	::memset(compressed_data_buffer, 0, BUFFER_SIZE);

	compressed_data_size = LZWBase::read_into_buffer((uchar*)compressed_data_buffer, BUFFER_SIZE, file_in);
#else
	compressed_data_buffer = memory;
	compressed_data_size = buffer_size;
#endif

	/*
	Allocate buffer to put the decompressed output, and initialize it to zero, in decompression initilization is 0 is not critical.
	*/
	decompression_buffer = (char*)::malloc(BUFFER_SIZE);
	::memset(decompression_buffer, 0, BUFFER_SIZE);

	/*
	Now instantiate the MyList class, to create the dictionary.
	*/
	dictionary = new MyList<storage_info>();
	LZWBase::push_default_elements(dictionary);

	LZWBase::step_byte_width(dictionary, &byte_width);
}

/*
The user of the class is responsible for deallocating the decompression_buffer
*/
LZWDecompress::~LZWDecompress()
{
#ifdef FILE_READ_BUFFER
	::free(compressed_data_buffer);
#endif // FILE_READ_BUFFER

	delete dictionary;
}

void LZWDecompress::decompress()
{
	const uint clear_code = 1 << this->default_byte_width
		, end_of_information = (1 << this->default_byte_width) + 1;

	while (compressed_data_pointer < compressed_data_size) {
		/*
		icode is the LZW code that is just a pointer to the location in the dictionary that it represents.
		*/
		uint icode = read_next_bits();

		/*
		Since switches requires to use a constant value, we have to use if/else-if statements.	
		*/
		if (icode == clear_code) {
			
			/*
			If encoder sends random clear code, we must be able to process them
			*/
			byte_width = this->default_byte_width;
			dictionary->clear();
			LZWBase::push_default_elements(dictionary);
			step_byte_width(dictionary, &byte_width);
			can_push = false;
		}
		else if (icode == end_of_information) // End of information must be the last code in the LZW Stream.
			return;
		else {
			read_compressed_stream(icode);
			step_byte_width(dictionary, &byte_width);
		}

		/*
		We have to do additional processing, if we are reading from files, just for the fact that the code can span over multiple bytes
		and we have to address this issue and move the memory accordingly, we cannot wait till all of the buffer is finished and the program starts
		reading random garbage values, we have a threshold of 32, so when 32 bytes in original buffer are left, we refill the buffer with fresh data.
		*/
#ifdef FILE_READ_BUILD
#define LONG_SIZE 32

		if (compressed_data_pointer >= (compressed_data_size - LONG_SIZE)
			&& (compressed_data_size - ((byte_width / 8) + ((byte_width % 8) > 0)) - 1) <= compressed_data_pointer) 
		{
			int min_byte = (compressed_data_size - compressed_data_pointer);
			::memcpy(compressed_data_buffer, compressed_data_buffer + compressed_data_pointer,
				min_byte);
			
			compressed_data_size = LZWBase::read_into_buffer(
				(unsigned char*)(compressed_data_buffer + min_byte),
				BUFFER_SIZE - min_byte,
				file_in) + min_byte;
			compressed_data_pointer = 0;
			if (compressed_data_size == 0
				|| (compressed_data_size == min_byte
					&& (((bit_pointer + byte_width) / this->default_byte_width)
						+ ((bit_pointer + byte_width) % this->default_byte_width > 0)) > min_byte)) break;
		}
#endif
	}
}

/*
Returns the decompressed stream, must be called after decompress();
*/
char * LZWDecompress::acquire_buffer(int *size)
{
	*size = decompression_buffer_pointer;
	return decompression_buffer;
}
