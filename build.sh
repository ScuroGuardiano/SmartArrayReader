if [ ! -f BUSE/libbuse.a ]; then
    cd BUSE
    make
    cd ..
fi


g++ main.cpp src/*.cpp -o main -LBUSE -lbuse -Iinclude -O3 -std=c++20