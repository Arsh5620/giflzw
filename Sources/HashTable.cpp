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

#include "../Headers/HashTable.h"
#include <cstring>
#include <iostream>

/*
While we use to have separate chaining for unoptimized code it is very poor as the code has to do a lot of memory allocations,
so I decided to go for open addressing which is much faster as there is no memory allocation which is quite expensive for a large set of elements
*/

/*
This function was used before any optimizations were done, so we were using string and this function is nice for hashing strings
------------------------------------------------------------------
typedef long long hash_t;
hash_t string_get_hash(char* _memory, hash_t size, hash_t modulus)
{
	hash_t _s = 7919;
	_s ^= *_memory;
	for (hash_t i = 0; i < size / sizeof(hash_t); i++) {
		hash_t c = *((hash_t*)_memory + i);
		_s = (_s << 5 | _s >> (32 - 5)) ^ (c + (i & 0xff));
	}
	return hash_t((_s) % modulus);
	//While the other function is fast for double integer hashing.
}
*/

/*
This hash function requires two inputs, one <<char>> and one <<integer>> along with the modulus

--------------
Properties of good hash function
1. Should be fast [our function is moderately fast]
2. Should generate random hash [our function do accomplish this quite nicely]
3. Should make use of all entropy available [our function does use all entroy]
*/
uint HashTable::get_hashcode(char _char, int _preval, uint modulus) {
	/*
	Primes:
	9929	10169	10399	10651	9931	10177	10427	10657
	*/

	/*
	Unoptimized:
	return((((_char * 10651) << 8) ^ _char ^ (_preval * 9931)) + _preval) % modulus;
	*/

	//Optimized:
	return((((_char * 2726656) + _char) ^ (_preval * 9931)) + _preval) & (modulus - 1);
}

/*
When the hash table is filled above a certain threshold that is HASH_FILL, we resize the hash_table
------------
In previous code we used to have used the prime numbers as the base for our hash table, but in our tests we did not found any significant improvement by using prime numbers with hash tables, so to speed up other parts of the program we decided to use hash table sizes as powers of two which allows for fast modulus.
*/
void HashTable::expand_table()
{
	uint original_size = hash_memory_size;
	hash_memory_size *= 2;// this->next_prime_above(hash_memory_size * HASH_EXPAND);

	hash_struct *new_memory = (hash_struct*)::malloc(hash_memory_size * sizeof(hash_struct));
	::memset(new_memory, 0, hash_memory_size * sizeof(hash_struct));

	hash_struct *memory = hash_memory;
	int i = original_size;

	//Scan through all the hash table buckets and re-map them into the new memory location
	while (i--) {
		/*
		How we used to re-map in the unoptimized code
		-------------------
		if (*memory != 0) {
			MyList<hash_struct> *h_temp = (*memory);
			for (uint q = 0; q < h_temp->size(); q++) {
				hash_struct h_s = *(h_temp->operator[](q));
				add_private(new_memory, h_s._char, h_s._preval, h_s.code);
			}
			delete *memory;
		}
		*/

		//First check if the bucket needs to be processed, by checking if the is_valid flag is set.
		if (memory->is_valid) {
			add_private(new_memory, memory->char_code, memory->previous_code, memory->code, memory->is_valid);
		}
		++memory;
	}

	//After we have processed all the buckets, we throw away the old memory(::free) and set the pointer to the new memory location.
	::free(hash_memory);
	hash_memory = new_memory;
	this->hash_update_size = (uint)(this->hash_memory_size*HASH_FILL);
}

/*
This method actually insert(s) an item into the hash_table by doing all the necessary hash calcuations and all other operations.
*/
void HashTable::add_private(hash_struct *source_memory, char _char, int _preval, uint code, char valid)
{
	/*
	First get the hash value generated for the given set of _char, and _preval and then map the value to the location in the hash table data structure.
	*/
	hash_struct *memory_location = (source_memory + get_hashcode(_char, _preval, hash_memory_size));

	//Allocate some space on stack to create the hash_struct data structure, that will be sent to the heap later.
	hash_struct hash;
	hash.char_code = _char;
	hash.previous_code = _preval;
	hash.code = code;
	hash.is_valid = valid;

	/*
	If the memory_location->valid is not 0 it means that the bucket is occupied and according to open addressing we must look for next available space,
	and keep on doing so until we find one.
	*/
	while (memory_location->is_valid) {
		++memory_location;

		//Once we have reached to the end of the buffer's size, we start again from the beginning.
		if (memory_location >= (source_memory + hash_memory_size))
			memory_location = source_memory;
	}
	/*
	If memory_location_>is_valid is 0, which mean the bucket is empty, just put the structure at that location
	*/
	*memory_location = hash;
	return;
}

/*
This function wraps add_private so we don't need to take care of additional parameters to the add_private function.
*/
void 
#ifdef _MSVC_VER
	__fastcall 
#endif
	HashTable::add(char _char, int _preval, uint code)
{
	add_private(hash_memory, _char, _preval, code, 1);

	/*
	If the hash_total_elements/hash_memory_size is above HASH_FILL then expand the table.
	*/
	if (hash_total_elements > hash_update_size)
		expand_table();

	++hash_total_elements;
}

/*
Adds the codes to hash table in such a way as they only work as a padding instead of being able to be used later
*/
void HashTable::add_special_codes(char _char, int _preval, uint code)
{
	add_private(hash_memory, _char, _preval, code, 2);

	if (hash_total_elements > hash_update_size) {
		expand_table();
	}
	++hash_total_elements;
}

/*
The most important(or one of) function(s) that actually scans through the hash table storage structure and returns the found structure.
*/
hash_struct HashTable::get(char character, unsigned int previous_code)
{
	hash_struct* hash_pointer = (hash_memory + get_hashcode(character, previous_code, hash_memory_size));
	while (hash_pointer->is_valid) {
		if (hash_pointer->is_valid == 1
			&& hash_pointer->previous_code == previous_code
			&& hash_pointer->char_code == character)
			return *hash_pointer;
		++hash_pointer;
		if (hash_pointer >= hash_memory + hash_memory_size) {
			hash_pointer = hash_memory;
		}
	}
	return { 0 };
}

/*
Returns the total number of elements that have been added to the hash table so far.
*/
uint HashTable::size()
{
	return uint(this->hash_total_elements);
}

/*
Purges and rebuilds the entire hash table.
*/
void HashTable::clear()
{
	if (hash_memory_size > (base_reset_size << 1))
	{
		::free(hash_memory);
		hash_memory = (hash_struct*)::malloc(base_reset_size * sizeof(hash_struct));
		::memset(hash_memory, 0, base_reset_size * sizeof(hash_struct));
		hash_memory_size = base_reset_size;
	}
	else
		::memset(hash_memory, 0, hash_memory_size * sizeof(hash_struct));
	hash_update_size = (uint)(this->hash_memory_size * HASH_FILL);
	hash_total_elements = 0;
}

/*
Instantiates the HashTable class with the given begin_size.
*/
HashTable::HashTable(int begin_size)
{
	hash_memory = (hash_struct*)::malloc(begin_size * sizeof(hash_struct));
	::memset(hash_memory, 0, begin_size * sizeof(hash_struct));
	hash_memory_size =
		base_reset_size = begin_size;
	hash_update_size = (uint)(this->hash_memory_size * HASH_FILL);
	hash_total_elements = 0;
}

/*
Frees the memory used by the hash table structure.
*/
HashTable::~HashTable()
{
	::free(hash_memory);
}
