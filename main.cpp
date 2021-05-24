#include <iostream>
#include <libusb.h>
#include "protocol.h"
#include <chrono>
#include <thread>
#include <cmath>
#include <vector>

bool device_matches_vendor_product(libusb_device *device, unsigned short idVendor, unsigned short idProduct)
{
    libusb_device_descriptor desc;
    libusb_get_device_descriptor(device, &desc);
    return idVendor == desc.idVendor && idProduct == desc.idProduct;
}

class Maestro {
    libusb_context *ctx;
    libusb_device **device_list;
    libusb_device_handle *device_handle;
    bool valid;

public:
    Maestro(): ctx(0), device_list(0), device_handle(0), valid(false)
    {
        valid = initialise();
    }
    bool initialise()
    {
        const unsigned short vendor_id = 0x1ffb;
        unsigned short product_id_array[]={0x0089, 0x008a, 0x008b, 0x008c};
        libusb_init(&ctx);
        int count=libusb_get_device_list(ctx, &device_list);
        for (int i=0; i<count; i++) {
            libusb_device *device = device_list[i];
            for (int id=0; id<4; id++) {
                if (device_matches_vendor_product(device, vendor_id, product_id_array[id])) {
                    libusb_open(device, &device_handle);
                    return true;
                }
            }
        }
        return false;
    }

    ~Maestro()
    {
        libusb_close(device_handle);
        libusb_free_device_list(device_list, 0);
        libusb_exit(ctx);
    }

    void set_target(int servo, int command)
    {
        libusb_control_transfer(device_handle, 0x40, REQUEST_SET_TARGET, command, servo, 0, 0, (ushort)5000);
    }

    void set_position(int servo, double pos)
    {
        // Position in degrees from -60 to 60
        int command = 4*(1472 + (pos/180)*(2400-544));
        if (command < 4*800) command = 4*800;
        if (command > 4*2144) command = 4*2144;
        set_target(servo, command);
    }

    void disable(int servo)
    {
        set_target(servo, 0);
    }
};

int main()
{
    Maestro m;

    int left_servo = 1;
    int right_servo = 2;
    std::vector<int> servos = {0, 3, 4, 5, 6, 7};

    auto start = std::chrono::system_clock::now();
    auto prev = start;
    double position = 0;
    int direction = 1;
    double speed = 20;
    double max_pos = 40;

    int i = 0;
    while(i<2 || position*direction < 0) // only stop once passes the mid point
    {
        prev = start;
        start = std::chrono::system_clock::now();
        std::chrono::duration<double> elapsed = start - prev;

        m.set_position(left_servo, position);
        m.set_position(right_servo, -position);
        for (int servo: servos)
            m.set_position(servo, position);

        position += direction * speed * elapsed.count();
        if (std::fabs(position) > max_pos) {
            position -= direction * speed * elapsed.count();
            direction = -direction;
            i++;
        }
    }
    m.disable(left_servo);
    m.disable(right_servo);
    for (int servo: servos) m.disable(servo);
    return 0;
}
