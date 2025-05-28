g++ -IC:\SFML-3.0.0\include -c main.cpp -o main.o
g++ -LC:\SFML-3.0.0\lib .\main.o -o game.exe -lmingw32 -lsfml-graphics -lsfml-window -lsfml-system -mwindows
./game.exe