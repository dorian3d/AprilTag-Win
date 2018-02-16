#include "usb_host.h"
#include <libusb-1.0/libusb.h>
#include <signal.h>  // capture sigint
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>  // sleep
#include <iostream>
#include <thread>
#include <string.h>
#include <stream_object.h>

#define USB_TIMEOUT 0

using namespace std;

static libusb_context *       context;
static libusb_device_handle * device_handle;

void usb_write_packet(const rc_packet_t & packet)
{
    int r, bytes_written;
    int bytes_to_write = sizeof(packet_header_t);
    r                  = libusb_bulk_transfer(device_handle, ENDPOINT_HOST_OUT,
                             (uint8_t *)&packet->header, bytes_to_write,
                             &bytes_written, USB_TIMEOUT);
    if(r) {
        cerr << "bulk write header failed " << r << "\n";
        exit(1);
    }

    bytes_to_write = packet->header.bytes - sizeof(packet_header_t);
    if (bytes_to_write == 0) return;
    r              = libusb_bulk_transfer(device_handle, ENDPOINT_HOST_OUT,
                             (uint8_t *)&packet->data, bytes_to_write,
                             &bytes_written, USB_TIMEOUT);
    if(r) {
        cerr << "bulk write packet failed " << r << "\n";
        exit(1);
    }
}

rc_packet_t usb_read_packet()
{
    int             r = 0;
    packet_header_t packet_header;

    int bytes_to_read = sizeof(packet_header_t);
    int bytes_read;
    r = libusb_bulk_transfer(device_handle, ENDPOINT_HOST_IN,
                             (uint8_t *)&packet_header, bytes_to_read,
                             &bytes_read, USB_TIMEOUT);
    if(r) {
        cerr << "bulk transfer failed " << r << "\n";
        exit(1);
    }

    rc_packet_t packet((packet_t *)malloc(packet_header.bytes), free);
    packet->header = packet_header;

    uint64_t packet_bytes = packet->header.bytes - sizeof(packet_header_t);
    if(packet_bytes) {
        r = libusb_bulk_transfer(device_handle, ENDPOINT_HOST_IN,
                                 (uint8_t *)&packet->data, packet_bytes,
                                 &bytes_read, USB_TIMEOUT);
        if(r) {
            cerr << "bulk transfer failed " << r << "\n";
            exit(1);
        }
    }
    return packet;
}

bool usb_send_control(uint8_t control_message, uint16_t value, uint16_t index,
                      unsigned int timeout = 0)
{
    int r = libusb_control_transfer(
            device_handle, USB_bmRequestType_VSC_HOST_TO_DEVICE,
            control_message, value, index, NULL, 0, timeout);
    if(r) {
        std::cerr << "Control transfer error" << r << "\n";
        exit(1);
        return false;
    }
    return true;
}

void usb_shutdown(int _unused)
{
    if(!device_handle) return;

    if(!usb_send_control(CONTROL_MESSAGE_STOP_AND_RESET, 0, 0, 500)) {
        fprintf(stderr, "Error: unable to stop usb cleanly, exiting\n");
    }

    libusb_release_interface(device_handle, 0);
    libusb_close(device_handle);
    libusb_exit(context);
    device_handle = NULL;
    context       = NULL;
}

void usb_init()
{
    libusb_init(&context);
    libusb_set_debug(context, 3);
    std::cout << "Opening device handle ";
    while(!device_handle) {
        device_handle = libusb_open_device_with_vid_pid(context, USB_VENDOR_ID,
                                                        USB_PRODUCT_ID);
        sleep(1);
        std::cout << ".";
    }
    std::cout << "\nDevice open\n";
    libusb_device* dev = libusb_get_device(device_handle);
    if(libusb_get_device_speed(dev) != LIBUSB_SPEED_SUPER)
        std::cout << "Warning: Device is not connected via USB3\n";

    if(libusb_claim_interface(device_handle, 0)) {
        std::cerr << "Failed to claim interface\n";
        exit(1);
    }
}

bool usb_read_interrupt_packet(rc_packet_t &fixed_pkt)
{
    int32_t bytes_read = 0;
    int r = libusb_interrupt_transfer(device_handle, ENDPOINT_HOST_INT_IN,
        (uint8_t *)fixed_pkt.get(), fixed_pkt->header.bytes, &bytes_read, USB_TIMEOUT);
    if(r || (bytes_read != (int32_t)fixed_pkt->header.bytes)) {
        cerr << "interrupt packet transfer failed " << r << " read " << bytes_read << " vs. expected " << fixed_pkt->header.bytes << "\n";
        return false;
    }
    return true;
}
