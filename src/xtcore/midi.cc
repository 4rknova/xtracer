#include "rtmidi/RtMidi.h"
#include "memutil.tml"
#include "config.h"
#include "log.h"
#include "midi.h"

namespace xtcore {
    namespace midi {

#if FEATURE_IS_INCLUDED(FEATURE_MIDI)

RtMidiIn *_midiin = 0;

int init()
{
    try   { _midiin = new RtMidiIn(); }
    catch (RtMidiError &error) {
        Log::handle().post_error("%s", error.std::exception::what());
        return -1;
    }

    return 0;
}

int deinit()
{
    xtcore::memory::safe_delete(_midiin);
    return 0;
}

int detect(devices_t *devs)
{
    if (!devs   ) return -3;
    if (!_midiin) return -2;

    size_t ports = _midiin->getPortCount();
    for (size_t i = 0; i < ports; ++i) {
        device_t dev;
        devs->devices.push_back(dev);
        device_t *d = &(devs->devices.back());

        d->port = i+1;
        try   { d->name = _midiin->getPortName(i); }
        catch (RtMidiError &error) {
            Log::handle().post_error("%s", error.std::exception::what());
            return -1;
        }
    }
}

int open(device_t *dev)
{
    if (!dev    ) return -3;
    if (!_midiin) return -2;

    _midiin->openPort(dev->port);

    // Don't ignore sysex, timing, or active sensing messages.
    _midiin->ignoreTypes(false, false, false);
}

int close(device_t *dev)
{
    if (!dev    ) return -3;
    if (!_midiin) return -2;

    _midiin->closePort();
}

#else

int init() {}
int deinit() {}
int detect(devices_t *devs) {}
int open(device_t *dev)  {}
int close(device_t *dev) {}
int read(device_t *dev)  {}

#endif /* FEATURE_MIDI */

    } /* namespace midi */
} /* namespace xtcore */
