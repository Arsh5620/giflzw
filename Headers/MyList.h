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

#include <malloc.h>
#include <memory.h>
/*
The MyList creates an object capable of storing the default_memory_elements number of type T objects.
So, if you know your objects are of huge size, and you don't need this much of elements to begin with, since the list auto expands as required
you can reduce the number, or if you need to store a lot of elements and want to avoid time spent on resize you can set this high
*/
#define DEFAULT_MEMORY_ELEMENTS	4096

/*
The factor by which list increases its size, 2 is a good number.
*/
#define SIZE_INCREASE 2

template<class T> class MyList
{
private:
	T * list_memory;
	unsigned int pointer = 0
		, size = 0;
public:

	/*
	Allocates memory to create appropriate structures requires to store all the object that will be pushed.
	*/
	MyList() {
		list_memory = (T*)::malloc(sizeof(T)*DEFAULT_MEMORY_ELEMENTS);
		size = DEFAULT_MEMORY_ELEMENTS;
	}

	/*
	free_list just calls ::free(list_memory); could have been inlined
	*/
	~MyList() {
		free_list();
	}

	/*
	Returns the <T> object that exists at index location.
	*/
	T *operator[] (int index) {
		return (T*)(list_memory + index);
	}

	void expand() {

		size *= SIZE_INCREASE;

		void *memory = 0;
		int copy_size = list_size() * sizeof(T),
			destination_size = size * sizeof(T);
		if ((memory = ::realloc(list_memory, destination_size)) == 0) {
			memory = ::malloc(copy_size);
			::memcpy(memory, list_memory, copy_size);
			::free(list_memory);
		}
		else ::memset(((char*)memory + copy_size), 0, destination_size - copy_size);

		list_memory = (T*)(memory);
	}

	/*
	Pushes the object of type T to the list memory
	*/
	void push(T object) {
		if (pointer == size) expand();
		::memcpy(list_memory + pointer, &object, sizeof(T));
		++pointer;
	}

	/*Returns the total number of object the list currently contains*/
	unsigned int list_size() {
		return this->pointer;
	}

	/*Deletes all the objects in the list and release and then re allocate memory to defaults*/
	void clear() {
		if(this->size>(DEFAULT_MEMORY_ELEMENTS<<1))
		{
			free_list();
			list_memory = (T*)::malloc(sizeof(T)*DEFAULT_MEMORY_ELEMENTS);
			size = DEFAULT_MEMORY_ELEMENTS;
		}
		else ::memset(list_memory, 0, sizeof(T)*this->size);
		pointer = 0;
	}

private:
	void free_list() {
		::free(list_memory);
	}
};
