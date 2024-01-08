if [ ! -f BUSE/libbuse.a ]; then
    cd BUSE
    make
    cd ..
fi

g++ hewlett-read.cpp src/*.cpp -o hewlett-read -LBUSE -lbuse -Iinclude -O3 -std=c++20
g++ packard-tell.cpp src/drive_reader.cpp -o packard-tell -Iinclude -O3 -std=c++20