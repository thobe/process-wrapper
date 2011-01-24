wrapper: src/wrapper.cpp
	g++ src/wrapper.cpp -m32 `wx-config --libs` `wx-config --cxxflags` -o wrapper
