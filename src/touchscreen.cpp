#include "touchscreen.hpp"
#include <string.h>

#define LOG(...)  \
    if(m_verbose) \
    printf(__VA_ARGS__)

cTouchScreen::cTouchScreen(bool verbose) : m_verbose(verbose)
{
    m_active = false;
    m_fd = 0;
    if((getuid()) != 0)
        fprintf(stderr, "You are not root! This may not work...\n");
    else
    {
        /* Open Device */
        m_fd = open(EVENT_DEVICE, O_RDONLY);
        if(m_fd == -1)
            fprintf(stderr, "%s is not a vaild device\n", EVENT_DEVICE);
        else
        {
            if(ioctl(m_fd, EVIOCGVERSION, &m_version))
                printf("evtest: can't get version");

            LOG("Input driver version is %d.%d.%d\n", (m_version >> 16),
                ((m_version >> 8) & 0xff), (m_version & 0xff));

            ioctl(m_fd, EVIOCGID, m_id);
            LOG("Input device ID: bus 0x%x vendor 0x%x product 0x%x version "
                "0x%x\n",
                m_id[ID_BUS], m_id[ID_VENDOR], m_id[ID_PRODUCT],
                m_id[ID_VERSION]);

            ioctl(m_fd, EVIOCGNAME(sizeof(m_name)), m_name);
            LOG("Input device name: \"%s\"\n", m_name);

            memset(m_bit, 0, sizeof(m_bit));
            ioctl(m_fd, EVIOCGBIT(0, EV_MAX), m_bit[0]);
            LOG("Supported events:\n");

            ioctl(m_fd, EVIOCGBIT(EV_ABS, KEY_MAX), m_bit[EV_ABS]);
            for(uint code = 0; code < KEY_MAX; code++)
                ioctl(m_fd, EVIOCGABS(code), m_abs_param[code]);

            m_mt_pos = new int32_t *[m_abs_param[ABS_MT_SLOT][ABS_PARAM_MAX]];
            for(int i = 0; i < m_abs_param[ABS_MT_SLOT][ABS_PARAM_MAX] + 1; i++)
                m_mt_pos[i] = new int32_t[3]{-1, -1};

            LOG("device file = %s\n", EVENT_DEVICE);
            LOG("device name = %s\n", m_name);
            m_active = true;

            FD_ZERO(&m_rdfs);
            FD_SET(m_fd, &m_rdfs);
            m_thread = new std::thread(&cTouchScreen::loop, this);
        }
    }
}

cTouchScreen::~cTouchScreen()
{
    if(m_fd > 0)
    {
        m_active = false;
        m_thread->join();
        close(m_fd);
    }
    m_fd = 0;
}

void *
cTouchScreen::loop(void *obj)
{
    while(reinterpret_cast<cTouchScreen *>(obj)->m_active)
        reinterpret_cast<cTouchScreen *>(obj)->readEv();
    return nullptr;
}

void
cTouchScreen::readEv()
{
    select(m_fd + 1, &m_rdfs, NULL, NULL, NULL);
    if(read(m_fd, &m_ev, ev_size) < ev_size)
        fprintf(stderr, "Error size when reading\n");
    else
    {
        switch(m_ev.type)
        {
        case EV_SYN:
        {
            break;
        }
        case EV_KEY:
        {
            break;
        }
        case EV_ABS:
        {
            m_abs_param[m_ev.code][ABS_PARAM_VALUE] = m_ev.value;
            switch(m_ev.code)
            {
            case ABS_X:
            case ABS_Y:
                m_abs_pos[m_ev.code - ABS_X] = m_ev.value;
                break;
            case ABS_PRESSURE:
                m_abs_pos[2] = m_ev.value;
                break;
            case ABS_MT_POSITION_X:
            case ABS_MT_POSITION_Y:
                m_mt_pos[m_abs_param[ABS_MT_SLOT][ABS_PARAM_VALUE]]
                        [m_ev.code - ABS_MT_POSITION_X] = m_ev.value;

                break;
            case ABS_MT_PRESSURE:
                m_mt_pos[m_abs_param[ABS_MT_SLOT][ABS_PARAM_VALUE]][2] =
                    m_ev.value;
                break;
            case ABS_MT_TRACKING_ID:
                if(m_ev.value == -1)
                {
                    m_mt_pos[m_abs_param[ABS_MT_SLOT][ABS_PARAM_VALUE]][0] = -1;
                    m_mt_pos[m_abs_param[ABS_MT_SLOT][ABS_PARAM_VALUE]][1] = -1;
                    m_mt_pos[m_abs_param[ABS_MT_SLOT][ABS_PARAM_VALUE]][2] = -1;
                }
                break;
            }

            break;
        }
        }
    }
}
