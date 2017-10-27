model: model.o
	g++ -g -o model model.o -L/usr/local/lib -lessentia -lfftw3 -lyaml -lavcodec -lavformat -lavutil -lsamplerate -ltag -lfftw3f -lavresample
model.o: model.cpp
	g++ -g -c -pipe -Wall -O3 -fPIC -std=c++11 -pthread -I/usr/local/include/essentia/ -I/usr/local/include/essentia/scheduler/ -I/usr/local/include/essentia/streaming/  -I/usr/local/include/essentia/utils -I/usr/include/taglib -I/usr/local/include/gaia2 -I/usr/include/qt4 -I/usr/include/qt4/QtCore -D__STDC_CONSTANT_MACROS model.cpp
clean:
	rm model.o
