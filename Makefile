# *_* MakeFile *_*

#
#    GIFLZWLib --
#    This library provides LZW compression and decompression routine for GIF stream compression.
#
#    Author: Arshdeep Singh, copyleft 2017.
#    LZW algorithm was originally created by Abraham Lempel, Jacob Ziv, and Terry Welch.
#    
#    Please see "LICENCE" to read the GPL.
#
#    This program is free software; you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation; either version 2 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License along
#    with this program; if not, write to the Free Software Foundation, Inc.,
#    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

SHELL=/bin/bash

CC=gcc
CPP=g++

FAST = -Ofast 
STANDARD = -std=c++11 
WARNINGS = -Wall
LINUXTEST = -Wl,-rpath,.
LIBRARYLOAD = -shared -fPIC
EXECFLAGS = 

ifdef DEBUG
CFLAGS = -g -O0 $(LIBRARYLOAD) $(STANDARD) $(WARNINGS)
else ifdef PROFILE
CFLAGS = -pg $(LIBRARYLOAD) $(WARNINGS)
EXECFLAGS = -pg
else
CFLAGS = $(FAST) $(STANDARD) $(LIBRARYLOAD) $(WARNINGS)
endif

SOURCES = ./Sources/LZWDecompress.cpp \
		  ./Sources/HashTable.cpp \
		  ./Sources/LZWBase.cpp \
		  ./Sources/LZWCompress.cpp

OBJECTS = ${SOURCES: .cpp=.o}

HEADERS =   ./Headers/MyList.h \
			./Headers/HashTable.h \
			./Headers/LZWBase.h \
			./Headers/LZWCompress.h \
			./Headers/LZWDecompress.h 

GIFLZWLib.so: $(SOURCES) $(HEADERS)
	$(CPP) $(CFLAGS) -o $@ $(SOURCES) 

clean:
	git clean -f

test: linux.cpp GIFLZWLib.so
	$(CPP) $(EXECFLAGS) $(FAST) $(LINUXTEST) -o $@ $^