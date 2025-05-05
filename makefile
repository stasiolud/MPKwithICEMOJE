all: build_slice build_system build_passenger build_tram

comp: build_system build_passenger build_tram

build_slice:
	slice2cpp mpk.ice
build_system:
	c++ -I. -DICE_CPP11_MAPPING -c mpk.cpp system.cpp -lpthread
	c++ -o system mpk.o system.o -lIce++11 -lpthread
build_passenger:
	c++ -I. -DICE_CPP11_MAPPING -c mpk.cpp passenger.cpp -lpthread
	c++ -o passenger mpk.o passenger.o -lIce++11 -lpthread
build_tram:
	c++ -I. -DICE_CPP11_MAPPING -c mpk.cpp tram.cpp -lpthread
	c++ -o tram mpk.o tram.o -lIce++11 -lpthread
clean:
	rm -f *.o system passenger tram mpk.cpp mpk.h