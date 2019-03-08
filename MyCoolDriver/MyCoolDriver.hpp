#include <IOKit/IOService.h>

class com_Glamdevelopment_driver_MyCoolDriver : public IOService {
OSDeclareDefaultStructors(com_Glamdevelopment_driver_MyCoolDriver)
public:
    virtual bool init(OSDictionary *dictionary = 0);
    virtual void free(void);
    virtual IOService *probe(IOService* provider, SInt32 *score);
    virtual bool start(IOService* provider);
    virtual void stop(IOService* provider);

    virtual IOReturn setPowerState(unsigned long powerStateOrdinal, IOService *whatDevice);
private:
    thread_call_t delayTimer;
    static void _disableTurboBoost(thread_call_param_t us, thread_call_param_t);
};
