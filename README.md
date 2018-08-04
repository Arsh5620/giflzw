# Lempel Ziv Welch Compression algorithm
The LZW compression routine is used to compress GIF image stream. This algorithm implementation is moderately optimized, and currently, we are working on error handling and testing.

# Example use of the library
<pre><code>#include "LZWCompression.h"

char buffer[512];
int compressed_size=0;

LZWCompress lzw(buffer, 512);
lzw.compress();
char *compressed_stream=lzw.acquire_buffer(&compressed_size);

::free(compressed);
</code></pre>

1. The <b>buffer</b> is the pointer to the data to be compressed.<br>
2. The <b>compressed_size</b> will store the size of the compressed data.<br>
3. The <b>compressed_stream</b> will store the handle to the compressed data.<br>
4. Also, the hardcoded value in the LZWCompress constructor(<b>512</b>) is the size of input(<i>uncompressed</i>) data.

After initializing the <b>LZWCompress</b>, the first call is made to the <b>compress</b> method, which actually performs all the compression, and then the <b>acquire_buffer</b> method is called which returns the handle to the compressed data.<br>

# Motivation
This project is created as I needed a moderately <i>optimized</i> <b>LZW compressor</b> for transcoding <b>GIF</b> files.<BR>
I searched online for some good LZW compressors but most of them were written in CSharp and were a bottleneck for performance.

# Installation
This library is intended to be a dynamically linked module.

For <b>Linux</b> you can use the makefile to build the shared object file, example uses below.
<pre><code>~/$ cd ./GIFLZWLib
~/GIFLZWLib/$ make #Will build only the shared object file.
~/GIFLZWLib/$ make test #Will build both the shared object file and the test executable.
~/GIFLZWLib/$ make debug #Will build the debug version of the shared object.
~/GIFLZWLib/$ make test-debug #Will build the debug version of both shared object and test module.
</code></pre>

For <b>Windows</b>, things are bit easier.
1. Open the project<sup>(.vcxproj)</sup> file. 
2. Press either Ctrl+Shift+B or go to Build->Build Solution.

# Optimizations
The program focuses more on speed than memory usage, sometimes we allocate twice as much RAM as actually required to defer future allocations.<BR>
Memory is kept low by reducing class instantiations, using open hashing instead of separate chaining.<BR>
  Program is made fast by allocating large chunks<sup>1</sup> of memory instead of a tiny but huge number of memory allocations<sup>2</sup>.<BR><BR>
> <i>Keeping memory low was quite tough, when we have to optimize for speed as a priority.</i>

  1. Large chunks refer to chunks that will store an array of structs and are of size enough big to store all such structures.
  2. Tiny allocations refers to allocations required to store one struct, and instantiating such struct with <b>new</b> for each one of the structure.
  
# Benchmarks
Before showing you some benchmarks, I would like to tell that the algorithm in general uses more RAM <sup>1</sup>, and compression results in bigger files <sup>2</sup> compared to LZ77. <BR>
<BR>
1. Because it is dictionary encoder, and we have to create a dictionary for 2^K items where K is code size, mostly 8. So the largest string in dictionary can take (2^K * ((2^K)+1))/2 bytes and for K=8 it is 8390656 bytes. That is why dictionary encoder requires more RAM, and it also depends upon input, if it is random and uncompressible, the RAM usage is higher, for highly compressible input the RAM usage is very low.

2. Because unlike LZSS used in most compressors, LZW has no way of optimizing away codes that are bigger than values they are replacing so if the match does not exists in dictionary we might be replacing 8 bits with 12 bits that still refers to the same code, and also since they are variable length, after we fill the table, we purge the table, requiring us to regenerate the table.
<hr>
The following details showing benchmarks are only for compression and the file read required during compression.

1. Random File - created using rand(), and the file size is 16MB flat.<BR>
    Average 553ms (used to be 2550ms), for compression including file read.<BR>
    Input size: 16777216 Bytes<BR>
    Compressed size: 21596070 (used to be 21512243) Bytes<BR>
    Compression ratio: 128.72 (used to be 128.22%)<BR>
    Compression speed: 30338000B/s 28.993MB/s (used to be 6623000B/sec 6.316MB/sec)<BR>
  
2. File with all bytes 0, Size 16MB flat.<BR>
    Average 178ms (used to be 550ms), for compression including file read.<BR>
    Input size: 16777216 Bytes<BR>
    Compressed size: 24900 (used to be 8585) Bytes<BR>
    Compression ratio: 0.15% (used to be 0.05%)<BR>
    Compression speed: 94254000B/s 89.888MB/s (used to be 30229000B/sec 28.829MB/sec)<BR>
  
The benchmark is on an Intel i5-6200U running at default clock. <BR>
<BR>
  
# License
This project uses GPLv2, refer to LICENSE file to read the license.
