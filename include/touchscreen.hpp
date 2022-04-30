#ifndef TOUCH_SCREEN_HPP_
#define TOUCH_SCREEN_HPP_

#include <fcntl.h>
#include <linux/input.h>
#include <thread>
#include <unistd.h>
#include <vector>

#define EVENT_DEVICE "/dev/input/event7"

#define BITS_PER_LONG (sizeof(long) * 8)
#define NBITS(x) ((((x)-1) / BITS_PER_LONG) + 1)

#define ABS_PARAM_VALUE 0
#define ABS_PARAM_MIN 1
#define ABS_PARAM_MAX 2
#define ABS_PARAM_FUZZ 3
#define ABS_PARAM_FLAT 4
#define ABS_PARAM_RESOLUTION 5

class cTouchScreen
{
    public:
    cTouchScreen(bool verbose = true);
    ~cTouchScreen();

    static void *
    loop(void *obj);

    void
    readEv();

    const int32_t *
    pos(uint32_t id = 0) const
    {
        id = (id < m_abs_param[ABS_MT_SLOT][ABS_PARAM_MAX] + 1)
                 ? id
                 : m_abs_param[ABS_MT_SLOT][ABS_PARAM_MAX] + 1;
        return m_mt_pos[id];
    };

    const int32_t
    max_pos_x() const
    {
        return m_abs_param[ABS_X][ABS_PARAM_MAX];
    };
    const int32_t
    max_pos_y() const
    {
        return m_abs_param[ABS_Y][ABS_PARAM_MAX];
    };
    const int32_t
    max_pos_p() const
    {
        return m_abs_param[ABS_PRESSURE][ABS_PARAM_MAX];
    };

    int32_t
    max_mt() const
    {
        return m_abs_param[ABS_MT_SLOT][ABS_PARAM_MAX] + 1;
    };

    private:
    std::thread *m_thread;
    bool m_active;
    int m_fd;
    fd_set m_rdfs;
    struct input_event m_ev;
    int32_t m_version;
    uint16_t m_id[4];
    int8_t m_name[256] = "Unknown";
    unsigned long m_bit[EV_MAX][NBITS(KEY_MAX)];
    int32_t m_abs_param[KEY_MAX][6];

    int32_t **m_mt_pos;
    int32_t m_abs_pos[3];
    bool m_verbose;

    const size_t ev_size = sizeof(struct input_event);
};

#endif // TOUCH_SCREEN_HPP_
