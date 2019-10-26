/* add your code here */

#include <IOKit/IOLib.h>
#include <mach/mach_types.h>
#include <libkern/libkern.h>
#include <i386/proc_reg.h>

#include "MyCoolDriver.hpp"

OSDefineMetaClassAndStructors(com_Glamdevelopment_driver_MyCoolDriver, IOService)

// Define the driver's superclass

#define super IOService

extern "C" {
extern void mp_rendezvous_no_intrs(
                                   void (*action_func)(void *),
                                   void *arg);
}

#define kMyNumberOfStates 2
#define kIOPMPowerOff 0

static IOPMPowerState myPowerStates[kMyNumberOfStates] = {
    {1, kIOPMPowerOff, kIOPMPowerOff, kIOPMPowerOff, 0, 0, 0, 0, 0, 0, 0, 0},
    {1, kIOPMPowerOn, kIOPMPowerOn, kIOPMPowerOn, 0, 0, 0, 0, 0, 0, 0, 0}
};

const uint64_t expectedFeatures  = 0x850089LL;
const uint64_t disableTurboBoost = 0x4000000000LL;

static void disable_tb(__unused void * param_not_used) {
    wrmsr64(MSR_IA32_MISC_ENABLE, rdmsr64(MSR_IA32_MISC_ENABLE) | disableTurboBoost);
}


bool com_Glamdevelopment_driver_MyCoolDriver::init(OSDictionary *dict) {
    bool result = super::init(dict);
    IOLog("[MyCoolDriver 1.0.6] Initializing MyCoolDriver..\n");
    return result;
}

void com_Glamdevelopment_driver_MyCoolDriver::free() {
    IOLog("[MyCoolDriver 1.0.6] Freeing\n");
    super::free();
}

IOService * com_Glamdevelopment_driver_MyCoolDriver::probe(IOService *provider, SInt32 *score) {
    IOService* result = super::probe(provider, score);
    IOLog("[MyCoolDriver 1.0.6] Probing..\n");
    return result;
}

void com_Glamdevelopment_driver_MyCoolDriver::_disableTurboBoost(thread_call_param_t us, thread_call_param_t)
{
    IOLog("[MyCoolDriver 1.0.6] [threadcall] executing.. 0\n" );
    uint64_t prev = rdmsr64(MSR_IA32_MISC_ENABLE);
    
//    if (prev & disableTurboBoost) {
//        IOLog("[MyCoolDriver 1.0.6] [threadcall] TurboBoost already disabled." );
//        return;
//    }
    
    IOLog("[MyCoolDriver 1.0.6] [threadcall] executing.. 1 val %llx \n", prev );
    mp_rendezvous_no_intrs(disable_tb, NULL);
    IOLog("[MyCoolDriver 1.0.6] [threadcall] executing.. 2\n" );
    uint64_t current = rdmsr64(MSR_IA32_MISC_ENABLE);
    
    printf("[MyCoolDriver 1.0.6] [threadcall] prev: %llx current: %llx check: %llx\n", prev, current, (current & disableTurboBoost) );
    printf("[MyCoolDriver 1.0.6] [threadcall] executing.. Done.\n" );
    IOLog("[MyCoolDriver 1.0.6] [threadcall] iolog prev: %llx current: %llx check: %llx\n", prev, current, (current & disableTurboBoost) );
    IOLog("[MyCoolDriver 1.0.6] [threadcall] iolog executing.. Done.\n" );
}

IOReturn com_Glamdevelopment_driver_MyCoolDriver::setPowerState ( unsigned long whichState, IOService * whatDevice )
// Note that it is safe to ignore the whatDevice parameter.
{
    if ( 0 == whichState ) {
        // Going to sleep. Perform state-saving tasks here.
    } else {
        // Waking up. Perform device initialization here.
        uint64_t prev = rdmsr64(MSR_IA32_MISC_ENABLE);
        mp_rendezvous_no_intrs(disable_tb, NULL);
        uint64_t current = rdmsr64(MSR_IA32_MISC_ENABLE);
        IOLog("[MyCoolDriver 1.0.6] [wake up] prev: %llx current: %llx check: %llx\n", prev, current, (current & disableTurboBoost) );
        
        if (delayTimer) {
            UInt64 deadline;
            clock_interval_to_deadline(2, kSecondScale, &deadline);
            thread_call_enter_delayed(delayTimer, deadline);
        }
        else {
            IOLog("[MyCoolDriver 1.0.6] [wake up] zero delay \n");
        }
    }
    
    return kIOPMAckImplied;
    
//    if ( done )
//        return kIOPMAckImplied;
//    else
//        return (/* a number of microseconds that represents the maximum time required to prepare for the state change */);
}

bool com_Glamdevelopment_driver_MyCoolDriver::start(IOService *provider)
{
    bool result = super::start(provider);
    IOLog("[MyCoolDriver 1.0.6] Starting\n");
    
    // Register for Wake event; Apple seems to like to re-enable TurboBoost after waking the maching up
    PMinit();
    provider->joinPMtree(this);
    registerPowerDriver (this, myPowerStates, kMyNumberOfStates);
    
    uint64_t prev = rdmsr64(MSR_IA32_MISC_ENABLE);
    mp_rendezvous_no_intrs(disable_tb, NULL);
    uint64_t current = rdmsr64(MSR_IA32_MISC_ENABLE);
    IOLog("[MyCoolDriver 1.0.6] prev: %llx current: %llx check: %llx\n", prev, current, (current & disableTurboBoost) );
    
    // Setup Another invocation
    delayTimer = thread_call_allocate(_disableTurboBoost, (thread_call_param_t) this);
    UInt64 deadline;
    clock_interval_to_deadline(5, kSecondScale, &deadline);
    thread_call_enter_delayed(delayTimer, deadline);
    
    return result;
}

void com_Glamdevelopment_driver_MyCoolDriver::stop(IOService *provider)
{
    IOLog("[MyCoolDriver 1.0.6] Stopping\n");
    
    if(delayTimer) {
        thread_call_cancel(delayTimer);
        thread_call_free(delayTimer);
    }

    PMstop();
    
    super::stop(provider);
}
