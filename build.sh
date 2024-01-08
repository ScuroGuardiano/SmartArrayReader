if [ ! -f BUSE/libbuse.a ]; then
    cd BUSE
    make
    cd ..
fi


g++ main.cpp src/smart_array_raid_5_reader.cpp -o main -LBUSE -lbuse -O3