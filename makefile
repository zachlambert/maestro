all:
	g++ -o main main.cpp -I/usr/include/libusb-1.0 -L/usr/lib/x86_64-linux-gnu/libusb-1.0.a /usr/lib/x86_64-linux-gnu/libusb-1.0.so
