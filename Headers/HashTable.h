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

/*
When the hash table is above a certain threshold, we must expand the table, so we expand it to the size <<multiplied>> by the hash_expand.
2 (Two) is a decent value.
*/
#define HASH_EXPAND 2

/*
prime_surety is a value that we feed into the rabin_miller primality test for which higher values mean stronger probability for being a prime.
But we don't need it to be 100% sure that it is a prime, 80-90% surety is enough for us.
increase the following variable to increase probability for the number to a prime at the expense of higher computation.
*/
//#define PRIME_SURETY 4

/*
For separate chaining a value of 80% is good value for hash table's expand fill value.
but since we are using open addressing we need to expand a bit faster at about 75% fill.
*/
#define HASH_FILL .75

/*
Can't write 'unsigned int' so now I type 'uint', compression compression everywhere. :)
*/
typedef unsigned int uint;

/*
Type: Structure
Explanation: For optimized code it is just a bit confusing, I will try my best, please read LZWBase.h's storage_info first.
char_code:		Just an ASCII code like an "A".
is_valid:		a character, used to set the type of the struct, is valid if above 0.
code:			the code for the current structure(string).
previous_code:  the previous structure's "code" value.
------------------------------------------------------
So since I am encoding right now, how do I find if this string already exists in our dictionary as there does not exists a actual string
but a structure that comes together to form multiple strings from the same base?

You start with the first ASCII code from the string, say I have string "ABC", and I want to check if it already exists in my hash table.
1)	"A":65, well this is the first string with previous_code set to -1 as unsigned int. Call get_hashcode with character=65, and previous_code=-1
	And well it finds it definitely as it the default element that is added when the hash table is created in first place.
2)	"B":66, Since it comes after "A" which has code "65", so if it has to exist in our hash table it must have previous_code of 65, and character = 66
	So call get_hashcode with those values and check if exists, but there is one thing that is different now, we set the special function level variable
	say "last_code"(not the real variable name) to the hash table's total elements inserted because it was last to be inserted to the table.
3)	"C":67, last character was "B":66 so previous_code can only be "last_code", so call get_hashcode with previous_code = "last_code" and character = 67,
	and voila if it exists it will be returned, so handle it appropriately and from thereon you can search large strings the same way.

Why this way?
Because now we don't have to scan through each and every element in the dictionary against the current string to check if the string exists.
optimizing the speed and memory both at the same time.
*/
struct hash_struct {
	char char_code;
	char is_valid;
	uint code;
	uint previous_code;
};

class HashTable
{
private:
	hash_struct * hash_memory;
	uint hash_memory_size
		, hash_update_size
		, hash_total_elements
		, base_reset_size;

	void expand_table();
	uint get_hashcode(char _char, int _preval, uint modulus);
	void add_private(hash_struct * source_memory, char _char, int _preval, uint code, char valid);
public:
	uint size();
	void clear();
	~HashTable();
	HashTable(int computed_size);
	/*
	If we are going to call this function millions of times, hell sure it must be __fastcall in registers
	*/
	void
#ifdef _MSVC_VER
 	__fastcall 
#endif
	add(char _char, int _preval, uint code);
	hash_struct get(char _char, unsigned int _preval);
	void add_special_codes(char _char, int _preval, uint code);
};

