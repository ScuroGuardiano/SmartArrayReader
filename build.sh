if [ ! -f /BUSE/libbuse.a ]; then
    cd BUSE
    make
    cd ..
fi


g++ main.cpp -o main -LBUSE -lbuse