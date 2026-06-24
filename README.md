# jpeg-compressor
Simple JPEG compressor in C++

To compile
> g++ src/*.cpp -o bin/main -I includes/

To run (compress)
> bin/main 0 image_name.png bitstream.data

To run (decompress)
> bin/main 1 bitstream.data image_name.png
