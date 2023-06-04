#ifndef TOUCH_SCREEN_HPP_
#define TOUCH_SCREEN_HPP_

#include <fcntl.h>
#include <linux/input.h>
#include <thread>
#include <unistd.h>
#include <vector>

#include "strANSIseq.hpp"
#include <iostream>
#include <string.h>

#define EVENT_DEVICE "/dev/input/event7"

#define BITS_PER_LONG (sizeof(long) * 8)
#define NBITS(x) ((((x)-1) / BITS_PER_LONG) + 1)

#define ABS_PARAM_VALUE 0
#define ABS_PARAM_MIN 1
#define ABS_PARAM_MAX 2
#define ABS_PARAM_FUZZ 3
#define ABS_PARAM_FLAT 4
#define ABS_PARAM_RESOLUTION 5

//to disable the touchscreen as a mouse : xinput set-prop id "Device Enabled" 0

class cTouchScreen : public ESC::CLI
{
    public:
    cTouchScreen(const char *path = "/dev/input/event7", int verbose = -1);
    ~cTouchScreen();

    static void *loop(void *obj);

    void readEv();

    const int32_t *pos(uint32_t id = 0) const
    {
        if(id < m_mt_nb_slot)
            return m_mt_pos[id];
        return nullptr;
    };

    const double *pos_rel(uint32_t id = 0) const
    {
        if(id < m_mt_nb_slot)
            return m_mt_pos_rel[id];
        return nullptr;
    };

    int32_t max_mt() const { return m_mt_nb_slot; };

    bool is_absolute() { return !m_INPUT_PROP_POINTER; };

    bool has_pressure() { return m_has_pressure; };

    bool m_active;

    private:
    std::thread *m_thread;
    int m_fd;
    fd_set m_rdfs;
    struct input_event m_ev;
    int32_t m_version;
    uint16_t m_id[4];
    int8_t m_name[256] = "Unknown";
    unsigned long m_bit[EV_MAX][NBITS(KEY_MAX)];
    unsigned long m_propbits[INPUT_PROP_MAX];
    int32_t m_abs_param[KEY_MAX][6];

    bool m_INPUT_PROP_POINTER;
    bool m_INPUT_PROP_BUTTONPAD;

    bool m_has_pressure;

    int32_t m_mt_nb_slot = 0;
    int32_t **m_mt_pos = nullptr;
    double **m_mt_pos_rel = nullptr;
    int32_t *m_mt_pressure = nullptr;
    double *m_mt_pressure_rel = nullptr;
    double m_pos_max[3];
    int32_t m_abs_pos[3];
    int32_t m_pressure_max;
    int32_t m_pressure;

    struct timeval m_tv = {.tv_sec = 2, .tv_usec = 0};

    const size_t ev_size = sizeof(struct input_event);
};

#endif // TOUCH_SCREEN_HPP_
