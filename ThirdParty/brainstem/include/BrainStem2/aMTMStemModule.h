/////////////////////////////////////////////////////////////////////
//                                                                 //
// file: aMTMUSBStem.h                                            //
//                                                                 //
/////////////////////////////////////////////////////////////////////
//                                                                 //
// description: Definition of the Acroname 40-pin module object.   //
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

#ifndef __aMTMStemModule_H__
#define __aMTMStemModule_H__

#include "BrainStem-all.h"
#include "aProtocoldefs.h"

#define aMTM_STEM_MODULE_BASE_ADDRESS                               4

#define aMTM_STEM_NUM_STORES                                        3
#define   aMTM_STEM_NUM_INTERNAL_SLOTS                             12
#define   aMTM_STEM_NUM_RAM_SLOTS                                   1
#define   aMTM_STEM_NUM_SD_SLOTS                                  255

#define aMTM_STEM_NUM_A2D                                           4
#define aMTM_STEM_NUM_APPS                                          4
#define aMTM_STEM_NUM_CLOCK                                         1
#define aMTM_STEM_NUM_DIG                                          15
#define aMTM_STEM_NUM_I2C                                           2
#define aMTM_STEM_NUM_POINTERS                                      4
#define aMTM_STEM_NUM_SERVOS                                        8
#define aMTM_STEM_NUM_TIMERS                                        8


using Acroname::BrainStem::Module;
using Acroname::BrainStem::Link;
using Acroname::BrainStem::AnalogClass;
using Acroname::BrainStem::AppClass;
using Acroname::BrainStem::ClockClass;
using Acroname::BrainStem::DigitalClass;
using Acroname::BrainStem::I2CClass;
using Acroname::BrainStem::PointerClass;
using Acroname::BrainStem::RCServoClass;
using Acroname::BrainStem::StoreClass;
using Acroname::BrainStem::SystemClass;
using Acroname::BrainStem::TimerClass;


class aMTMStemModule : public Module
{
public:

    aMTMStemModule(const uint8_t module = aMTM_STEM_MODULE_BASE_ADDRESS,
                   bool bAutoNetworking = true) :
    Module(module, bAutoNetworking)
    {
        analog[0].init(this, 0);
        analog[1].init(this, 1);
        analog[2].init(this, 2);
        analog[3].init(this, 3);
        
        app[0].init(this, 0);
        app[1].init(this, 1);
        app[2].init(this, 2);
        app[3].init(this, 3);
        
        clock.init(this, 0);
        
        digital[0].init(this, 0);
        digital[1].init(this, 1);
        digital[2].init(this, 2);
        digital[3].init(this, 3);
        digital[4].init(this, 4);
        digital[5].init(this, 5);
        digital[6].init(this, 6);
        digital[7].init(this, 7);
        digital[8].init(this, 8);
        digital[9].init(this, 9);
        digital[10].init(this, 10);
        digital[11].init(this, 11);
        digital[12].init(this, 12);
        digital[13].init(this, 13);
        digital[14].init(this, 14);
        
        i2c[0].init(this,0);
        i2c[1].init(this,1);
        
        pointer[0].init(this, 0);
        pointer[1].init(this, 1);
        pointer[2].init(this, 2);
        pointer[3].init(this, 3);
        
        servo[0].init(this, 0);
        servo[1].init(this, 1);
        servo[2].init(this, 2);
        servo[3].init(this, 3);
        servo[4].init(this, 4);
        servo[5].init(this, 5);
        servo[6].init(this, 6);
        servo[7].init(this, 7);
        
        store[storeInternalStore].init(this, storeInternalStore);
        store[storeRAMStore].init(this, storeRAMStore);
        store[storeSDStore].init(this, storeSDStore);
        
        system.init(this, 0);
        
        timer[0].init(this, 0);
        timer[1].init(this, 1);
        timer[2].init(this, 2);
        timer[3].init(this, 3);
        timer[4].init(this, 4);
        timer[5].init(this, 5);
        timer[6].init(this, 6);
        timer[7].init(this, 7);
        
    }
    
    AnalogClass analog[aMTM_STEM_NUM_A2D];
    AppClass app[aMTM_STEM_NUM_APPS];
    ClockClass clock;
    DigitalClass digital[aMTM_STEM_NUM_DIG];
    I2CClass i2c[aMTM_STEM_NUM_I2C];
    PointerClass pointer[aMTM_STEM_NUM_POINTERS];
    RCServoClass servo[aMTM_STEM_NUM_SERVOS];
    StoreClass store[aMTM_STEM_NUM_STORES];
    SystemClass system;
    TimerClass timer[aMTM_STEM_NUM_TIMERS];
   
    
};

#endif
