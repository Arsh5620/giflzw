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

#include "../Headers/LZWBase.h"

/*
Checks if the given pointer falls within the boundary of the buffer given the size of the buffer, if not then expand the buffer.
Small buffers are expanded by the factor of 4 while the large buffers are expanded by factor of 2.
*/
void LZWBase::check_to_extend(char **buffer, int *size, int pointer) {
	if (pointer == (*size - 1)) {
		if (*size < (1 << 18))
			*size <<= 2 * BUFFER_GROW_SIZE;
		else
			*size <<= BUFFER_GROW_SIZE;
		*buffer = (char*)LZWBase::extend_buffer(*buffer, pointer, *size);
	}
}

/*
Reads a file into the buffer given all the required information.
*/
int LZWBase::read_into_buffer(unsigned char *buffer, int size, ::FILE *file) {
	return (int)::fread((void*)buffer, sizeof(char), size, file);
}

/*
LZWBase can be instantiated without any parameters as the only parameter it requires is optional and has an implicit value.
*/
LZWBase::LZWBase(int start_width)
{
	this->byte_default_start = start_width;
}

/*
When we need to expand the memory we provide the pointer of the buffer to this function, and this function copies all the buffer into the newly
allocated memory of size(size) and sets the rest of memory upto size(new_size) to 0.
----------------------------------
What do this function do?
	First we allocate a new_memory value that will keep the address of the memory location that we need to return;
	And then we try to call reallocate to automatically reallocate the memory for us if possible, and we check the return value for nullptr,
	If the reallocation was success we ::memset the trailing buffer after the size(size) until size(new_size), a total of (new_size-size) bytes to 0.
	Else, if the reallocation was not possible, then we allocate a new memory location of size(new_size)
	and ::memset the new memory to 0, and copy over the previous buffer.
	Finally we free the existing buffer and return the buffer pointer.
*/
void *LZWBase::extend_buffer(void *buffer, int size, int new_size) {
	void *new_memory;
	if ((new_memory = ::realloc(buffer, new_size)) == nullptr) {
		new_memory = ::malloc(new_size);
		::memset(new_memory, 0, new_size);
		::memcpy(new_memory, buffer, size);
		::free(buffer);
	}
	else ::memset(((char*)new_memory + size), 0, new_size - size);
	return(new_memory);
}

/*
1/2: Pushes the default items (usually whole ASCII table) to the MyList before processing any input.
*/
void LZWBase::push_default_elements(MyList<storage_info> *dictionary) {
	storage_info si = { 0 };
	//Pushes all the required codes to the dictionary
	for (int i = 0; i < (1 << this->byte_default_start); i++) {
		si.char_code = (char)i;
		si.previous_code = -1;
		si.size = 1;
		(*dictionary).push(si);
	}
	//We do an extra 2 push for clear_code and end_of_information code, which do not need to have any thing special but is kind of a padding in the arrary.
	(*dictionary).push(si);
	(*dictionary).push(si);
}

/*
2/2: Pushes the default items (usually whole ASCII table) to the HashTable before processing any input.
*/
void LZWBase::push_default_elements(HashTable *table)
{
	//Add default ascii codes to the hash table, again the full ASCII table most likely
	for (int i = 0; i < (1 << this->byte_default_start); i++) {
		(*table).add((char)i, -1, i);
	}
	//But in hash table we add the special codes in to the table differently, well, we don't even need to bother adding this to hash table
	//As in hash table we don't have any indexes :TODO: pending
	for (int i = 0; i < 2; i++) {
		(*table).add_special_codes((char)i, -1, i);
	}
}

/*
1/2: Steps up the byte_width variable when the dictionary size is at the boundary of the size, as the decompression is always one step behind the compression
So we step up one step later than the compressor alternative, also when the byte_width is bigger than max_allowed we flush the list and re-push the default elements
*/
bool LZWBase::step_byte_width(MyList<storage_info> *dictionary, char *byte_width) {
	if (dictionary->list_size() >= (unsigned int)(1 << (*byte_width))) {
		++(*byte_width);
		if (*byte_width > MAX_BYTE_LEN) {
			*byte_width = (char)(this->byte_default_start);
			dictionary->clear();
			LZWBase::push_default_elements(dictionary);
			step_byte_width(dictionary, byte_width);
			return true;
		}
	}
	return false;
}

/*
2/2: Steps up byte_width when table has more elements than the (1<<byte_width) can store,
contrary to dictionary the hash table has default sizes of huge numbers so we need to call this function with the table's counter for number of elements it contain
*/
bool LZWBase::step_byte_width(HashTable *table, uchar *byte_width)
{
	if (table->size() > (unsigned int)(1 << (*byte_width))) {
		++(*byte_width);
		if (*byte_width >= MAX_BYTE_LEN) {
			return true;
		}
	}
	return false;
}

void LZWBase::manual_hash_clean(HashTable *table, uchar *byte_width)
{
	*byte_width = (char)(this->byte_default_start);
	table->clear();
	LZWBase::push_default_elements(table);
	step_byte_width(table, byte_width);
}
