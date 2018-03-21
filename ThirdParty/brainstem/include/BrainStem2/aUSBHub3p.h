/////////////////////////////////////////////////////////////////////
//                                                                 //
// file: aUSBHub3p.h	 	  	                                   //
//                                                                 //
/////////////////////////////////////////////////////////////////////
//                                                                 //
// description: USBHub3p C++ Module object.                        //
//                                                                 //
// build number: source                                            //
//                                                                 //
/////////////////////////////////////////////////////////////////////
//                                                                 //
// Copyright 1994-2015. Acroname Inc.                              //
//                                                                 //
// This software is the property of Acroname Inc.  Any             //
// distribution, sale, transmission, or re-use of this code is     //
// strictly forbidden except with permission from Acroname Inc.    //
//                                                                 //
// To the full extent allowed by law, Acroname Inc. also excludes  //
// for itself and its suppliers any liability, whether based in    //
// contract or tort (including negligence), for direct,            //
// incidental, consequential, indirect, special, or punitive       //
// damages of any kind, or for loss of revenue or profits, loss of //
// business, loss of information or data, or other financial loss  //
// arising out of or in connection with this software, even if     //
// Acroname Inc. has been advised of the possibility of such       //
// damages.                                                        //
//                                                                 //
// Acroname Inc.                                                   //
// www.acroname.com                                                //
// 720-564-0373                                                    //
//                                                                 //
/////////////////////////////////////////////////////////////////////

#ifndef __aUSBHub3p_H__
#define __aUSBHub3p_H__

#include "BrainStem-all.h"
#include "aProtocoldefs.h"

#define aUSBHUB3P_MODULE                                          6


#define aUSBHUB3P_NUM_APPS                                        4
#define aUSBHUB3P_NUM_POINTERS                                    4
#define aUSBHUB3P_NUM_STORES                                      2
#define   aUSBHUB3P_NUM_INTERNAL_SLOTS                           12
#define   aUSBHUB3P_NUM_RAM_SLOTS                                 1
#define aUSBHUB3P_NUM_TIMERS                                      8
#define aUSBHUB3P_NUM_USB                                         1
#define aUSBHUB3P_NUM_USB_PORTS                                   8

// Bit defines for port state UInt32
// use _BIT(X) from aDefs.h to get bit value.
// i.e if (state & _BIT(aUSBHUB3P_USB_VBUS_ENABLED))
#define aUSBHUB3P_USB_VBUS_ENABLED                                0
#define aUSBHUB3P_USB2_DATA_ENABLED                               1
#define aUSBHUB3P_USB3_DATA_ENABLED                               3
#define aUSBHUB3P_USB_SPEED_USB2                                 11
#define aUSBHUB3P_USB_SPEED_USB3                                 12
#define aUSBHUB3P_USB_ERROR_FLAG                                 19
#define aUSBHUB3P_USB2_BOOST_ENABLED                             20
#define aUSBHUB3P_DEVICE_ATTACHED                                23

// Bit defines for port error UInt32
// use _BIT(X) from aDefs.h to get bit value.
// i.e if (error & _BIT(aUSBHUB3P_ERROR_VBUS_OVERCURRENT))
#define aUSBHUB3P_ERROR_VBUS_OVERCURRENT                          0
#define aUSBHUB3P_ERROR_VBUS_BACKDRIVE                            1
#define aUSBHUB3P_ERROR_HUB_POWER                                 2
#define aUSBHUB3P_ERROR_OVER_TEMPERATURE                          3

using Acroname::BrainStem::Module;
using Acroname::BrainStem::Link;
using Acroname::BrainStem::AppClass;
using Acroname::BrainStem::PointerClass;
using Acroname::BrainStem::StoreClass;
using Acroname::BrainStem::SystemClass;
using Acroname::BrainStem::TemperatureClass;
using Acroname::BrainStem::TimerClass;
using Acroname::BrainStem::USBClass;

class aUSBHub3p : public Module
{
public:

    aUSBHub3p(const uint8_t module = aUSBHUB3P_MODULE,
              bool bAutoNetworking = true) :
    Module(module, bAutoNetworking)
    {
        
        app[0].init(this, 0);
        app[1].init(this, 1);
        app[2].init(this, 2);
        app[3].init(this, 3);
        
        pointer[0].init(this, 0);
        pointer[1].init(this, 1);
        pointer[2].init(this, 2);
        pointer[3].init(this, 3);
        
        store[storeInternalStore].init(this, storeInternalStore);
        store[storeRAMStore].init(this, storeRAMStore);
        
        system.init(this, 0);
        
        temperature.init(this, 0);
        timer[0].init(this, 0);
        timer[1].init(this, 1);
        timer[2].init(this, 2);
        timer[3].init(this, 3);
        timer[4].init(this, 4);
        timer[5].init(this, 5);
        timer[6].init(this, 6);
        timer[7].init(this, 7);
        
        usb.init(this, 0);
    }
    
    AppClass app[aUSBHUB3P_NUM_APPS];
    PointerClass pointer[aUSBHUB3P_NUM_POINTERS];
    StoreClass store[aUSBHUB3P_NUM_STORES];
    SystemClass system;
    TemperatureClass temperature;
    TimerClass timer[aUSBHUB3P_NUM_TIMERS];
    USBClass usb;
    
};

#endif /* __aUSBHub3p_H__ */
