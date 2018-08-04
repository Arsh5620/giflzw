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

#include "../Headers/LZWCompress.h"

//The code has been drastically reduced in the size, as we progress through our optimizations

/*
Writes N bits such that 0<N<=32 to the output buffer, and sets the appropriate variables.
*/
void __inline LZWCompress::write_multibyte_buffer(uint information)
{
	char iteration_a = (this->bit_pointer + byte_width)
		, iteration_b = (iteration_a >> 3) + ((iteration_a & 0x7) > 0) // Optimized(no speed gains in testing) from: "(iteration_a / 8) + (iteration_a % 8 > 0);", since division is expensive
		, bits_written = 0; // Stores the number of the bits that have already been written

	/*
	We know we need, i, j forever so making them static across functions saves a bit of time(~10 of ms)
	*/
	static char i = 0, j = 0;
	for (i = 0, j = iteration_b - 1; i < iteration_b; ++i) {
		char put_here = (8 - this->bit_pointer) // The number of bits that can be written to the byte we are currently on : ( 8 - already_used_bits )
			, leaf; // leaf actually stores the data we will write ("OR") to the current byte

		/*
		When writing data over multiple byte boundaries with some code size N we have to process different byte boundaries differently
		Eg: write 512 to buffer starting at index 0, we have to write 256 to first byte and 0 to first bit of second byte. here we have to process
			the 2nd byte different.
		*/
		if (i == j) {
			char size = (byte_width - bits_written)
				, lshift = bits_written - this->bit_pointer;

			/*
			Optimized from: "char lshift = byte_width - (size + *bit_pointer);"
			We know that the following will work, Simplifying equations:
			------------------------------------
			byte_width - ((byte_width - bits_written) + *bit_pointer)
			byte_width - (byte_width - bits_written) -*bit_pointer
			byte_width - byte_width + bits_written - *bit_pointer
			bits_written - *bit_pointer;
			*/

			leaf = (char)(information >> lshift);
			bits_written += size;
			this->bit_pointer += size;
		}
		else {
			bits_written += put_here;
			/*
			How much we shift depends upon how much space we have left, so 8 - number of bits already written
			*/
			char shift = 8 - (bits_written);

			/*
			How can it be negative? Because we are doing big endian encoding, anyways it does gets negative sometimes.
			*/
			leaf = (char)(shift & 0x80 //No speed gain from "shift < 0"
				? information >> -shift
				: information << shift);
			this->bit_pointer += put_here;
		}

		/*
		We are writing to the compression buffer here, so this is where all your compressed data goes through no matter what.
		*/
		*(this->compression_buffer + this->compression_buffer_pointer) |= leaf; // Minor speed gain over array[index]=value;

		if (this->bit_pointer >> 3) { // Optimized from:(no speed gains tested) "if(*bit_pointer>8)"
			++(this->compression_buffer_pointer);
			this->bit_pointer &= 0x7; // Optimized from: "*(bit_pointer)%=8;", compiler would have done this optimization too.
		}

		int next_pointer = this->compression_buffer_pointer; // Checks the boundary for compression buffer and call expand function if required.
		if (next_pointer == (this->compression_buffer_size - 1))
			LZWBase::check_to_extend(
			(char**)&compression_buffer
				, &this->compression_buffer_size
				, next_pointer);
	}
}

/*
Constructor: different constructor(s) for different build configuration(s)
*/
LZWCompress::LZWCompress
#ifdef FILE_READ_BUILD
(std::FILE *file, int start_width)
#else
(uchar *input_stream, int buffer_size, int start_width)
#endif
	: LZWBase(start_width)
{
	this->default_byte_width =
		this->byte_width = (char)start_width;

	/*
	Allocate some default buffer of size BUFFER_SIZE for compression_buffer
	*/
	compression_buffer = (uchar*)::malloc(BUFFER_SIZE);
	/*
	Also make sure it is Zeroed, because the "OR" operations that are going to take place later will not be very pleased to find garbage in the string.
	*/
	::memset(compression_buffer, 0, BUFFER_SIZE);

#ifdef FILE_READ_BUILD
	this->file_in = file;

	/*
	Get the size of the file, so we will know when to terminate the loop
	*/
	::fseek(file_in, 0, SEEK_END);
	file_size = ::ftell(file_in);
	::fseek(file_in, 0, SEEK_SET);

	buffer_size = LZWBase::read_into_buffer(buffer, sizeof buffer, file_in);
#else
	buffer = input_stream;
	this->buffer_size = buffer_size;
#endif

	/*
	Precompute the hash table's size so it is large enough to store a lot of entities before having to expand
	*/
	int compute = //HashTable::next_prime_above(
		(int)((1 << (MAX_BYTE_LEN > 18 ? 19 : (MAX_BYTE_LEN + 1))));
	//);
	table = new HashTable(compute);
	push_default_elements(table);
}

/*
Since the optimized version only have one giant size memory allocation, and that too is for the table only, so delete the table
Also deallocating the compression_buffer is responsibility of the user of the class.
*/
LZWCompress::~LZWCompress()
{
	delete table;
}

/*
This is the function where all of your compression magic happens :)
Also, you can safely discard the return type.
*/
int LZWCompress::compress()
{
	/*
	When we begin compressing, we find the char_code for first character in the string and use it to initialize our previous_code.
	*/
	int previous_code = (uchar)(table->get(*buffer, (uint)-1).char_code);
	++buffer_pointer;

	LZWBase::step_byte_width(table, &byte_width);
	/*
	GIF LZW Stream starts by sending out the clear code to the output stream.
	*/
	write_multibyte_buffer((1 << this->default_byte_width));

#ifdef FILE_READ_BUILD
	int iterate = 0;
#endif // FILE_READ_BUILD

	char last_char_code = 0;

	/*
	While we have data to process, process the data.
	Oh!, just in case you need to have the ability to terminate the processing of your data, you can add a kill switch here.
	*/
	while (buffer_pointer < buffer_size) {
		/*
		We call hash table's get method which call get_hash and maps the return value to the location in the hash table's memory, and return a value if the code exists.
		*/
		hash_struct element = table->get(buffer[buffer_pointer], previous_code);

		/*
		element.is_valid==1 means that the string already exists in the hash table, and we can continue to search for string with larger size.
		*/
		if (element.is_valid) {
			/*
			If the string exists we set the previous_code to the current "element"'s code and last_char_code to its "char_code",
			but we only use previous_code to look for string(s) and last_char_code is used in compression, such that a bigger string is not found.
			*/
			previous_code = element.code;
			last_char_code = element.char_code;
		}
		else {
			if (
#ifdef FILE_READ_BUILD
				iterate != this->file_size
				||
#endif
				previous_code > -1) {
				last_char_code = buffer[buffer_pointer];

				/*
				Now since we have came across a sequence that did not exist in our dictionary already
				example: we had "ABC", which existed in the hash table, and we tried to check if "ABCD" exists,
				which did not exits, so now we have to add it to our hashtable.
				*/
				table->add(last_char_code, previous_code, table->size());
			}

			/*
			We always write the code for the last found match, since this branch will only be taken when there is a match failure.
			*/
			write_multibyte_buffer(previous_code);

			previous_code = buffer[buffer_pointer];

			/*
			Now try to check if the byte_width is consistent, or it it needs to be incremented or reset.
			*/
			if (LZWBase::step_byte_width(table, &byte_width)) {
				write_multibyte_buffer(1 << this->default_byte_width);
				manual_hash_clean(table, &byte_width);
			}
			//When you purge the hash table, write a clear code to the output so that the decoder knows it has to clear the dictionary too

		}
		++buffer_pointer;

#ifdef FILE_READ_BUILD
		/*
		Now if we are reading directly from file, we have to check that the buffer_pointer does not go outside the boundary and we have to update
		the buffer, when we are at the boundary.
		*/
		if (buffer_pointer >= (buffer_size)) {
			buffer_size = LZWBase::read_into_buffer(buffer, BUFFER_SIZE, file_in);
			if (buffer_size == 0) break; // if buffer_size = 0, we are at the end of the file, just break the loop.
			buffer_pointer = 0;
		}
		++iterate;
#endif // FILE_READ_BUILD

	}

	/*
	if previous code is greater than -1 there is some unwritten information, that needs to be written to output.
	*/
	if (previous_code > -1) {
		write_multibyte_buffer(previous_code);
	}

	//Finally write the End of Information.
	write_multibyte_buffer((1 << this->default_byte_width) + 1);

	if (bit_pointer) compression_buffer_pointer++;
	return (compression_buffer_pointer);
}

/*
Returns the buffer information, and must be called after you have called compress()
*/
unsigned char * LZWCompress::acquire_buffer(int *size)
{
	*size = compression_buffer_pointer;
	return compression_buffer;
}
