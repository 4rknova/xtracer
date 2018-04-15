#ifndef XTCORE_MIDI_H_INCLUDED
#define XTCORE_MIDI_H_INCLUDED

#include <string>
#include <vector>

namespace xtcore {
    namespace midi {

typedef struct {
    int         port;
    std::string name;
} device_t;

typedef struct {
    std::vector<device_t> devices;
} devices_t;

int init();
int deinit();
int detect(devices_t *devs);
int open(device_t *dev);
int close(device_t *dev);
int read(device_t *dev);

    } /* namespace midi */
} /* namespace xtcore */

#endif /*  XTCORE_MIDI_H_INCLUDED */
