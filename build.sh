if [ ! -f BUSE/libbuse.a ]; then
    cd BUSE
    make
    cd ..
fi

g++ hewlett-read.cpp src/drive_reader.cpp src/smart_array*.cpp -o hewlett-read -LBUSE -lbuse -Iinclude -O3 -std=c++23
g++ packard-tell.cpp src/drive_reader.cpp src/metadata_parser.cpp -o packard-tell -Iinclude -O3 -std=c++23