g++ -ggdb -std=c++11 *.cpp
./a.out > example.dot
dot -Tpng example.dot -o example.png
xdg-open example.png
