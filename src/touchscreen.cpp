#include "touchscreen.hpp"

using namespace ESC;

cTouchScreen::cTouchScreen(int verbose) : CLI(verbose, "TouchScreen")
{
    logln("Initializing.", true);
    m_active = false;
    m_fd = 0;
    if((getuid()) != 0)
        logln(fstr("[INFO]", {BOLD}) + "Missing root privileges.");
}

cTouchScreen::~cTouchScreen()
{
    if(m_fd > 0)
    {
        log("Closing" + fstr("...", {BLINK_SLOW}));
        m_active = false;
        m_thread->join();
        close(m_fd);
        logln("\b\b\b" + fstr(" OK", {BOLD, FG_GREEN}));
    }
    m_fd = 0;
}

void cTouchScreen::connect(const char *path)
{
    m_fd = open(path, O_RDONLY);
    if(m_fd == -1)
    {
        logln("Scanning input for \"" + std::string(path) + "\"", true);
        std::string dev_path;
        std::string dev_name = path;
        for(int i = 0; i < 40; i++)
        {
            dev_path = "/dev/input/event" + std::to_string(i);
            m_fd = open(dev_path.c_str(), O_RDWR);
            log("Checking " + dev_path, true);
            if(m_fd > 0)
            {
                char test[256];
                ioctl(m_fd, EVIOCGNAME(sizeof(test)), test);
                log(": " + std::string(test) + "\n", false);
                if(dev_name == std::string(test))
                    break;
                close(m_fd);
                m_fd = -1;
            }
        }
        m_fd = open(dev_path.c_str(), O_RDWR);
    }
    if(m_fd != -1)
    {
        if(ioctl(m_fd, EVIOCGVERSION, &m_version))
            logln("Can't get version");

        ioctl(m_fd, EVIOCGNAME(sizeof(m_name)), m_name);
        logln(ESC::fstr("Touchscreen found:\t", {BOLD, FG_GREEN}), true);
        logln("Name: " + std::string((char *)m_name));
        logln("File: " + std::string(path));

        ioctl(m_fd, EVIOCGID, m_id);
        char id[100];
        sprintf(id,
                "ID: 0x%x(bus)  0x%x(vendor) 0x%x(product) "
                "0x%x(version)",
                m_id[ID_BUS], m_id[ID_VENDOR], m_id[ID_PRODUCT],
                m_id[ID_VERSION]);
        logln(id);
        logln("Version:" + std::to_string(m_version >> 16) + ":" +
              std::to_string((m_version >> 8) & 0xff) + ":" +
              std::to_string(m_version & 0xff));

        memset(m_bit, 0, sizeof(m_bit));
        ioctl(m_fd, EVIOCGBIT(0, EV_MAX), m_bit[0]);

        ioctl(m_fd, EVIOCGBIT(EV_ABS, KEY_MAX), m_bit[EV_ABS]);
        for(uint code = 0; code < KEY_MAX; code++)
            ioctl(m_fd, EVIOCGABS(code), m_abs_param[code]);

        m_mt_nb_slot = m_abs_param[ABS_MT_SLOT][ABS_PARAM_MAX] + 1;
        m_mt_pos = new int32_t *[m_mt_nb_slot];
        m_mt_pos_rel = new double *[m_mt_nb_slot];
        m_mt_pressure = new int32_t[m_mt_nb_slot];
        m_mt_pressure_rel = new double[m_mt_nb_slot];
        for(int i = 0; i < m_mt_nb_slot; i++)
        {
            m_mt_pos[i] = new int32_t[3]{-1, -1, -1};
            m_mt_pos_rel[i] = new double[3]{-1, -1, -1};
        }

        for(int i = 0; i < 2; i++)
            m_pos_max[i] = m_abs_param[ABS_MT_POSITION_X + i][ABS_PARAM_MAX];

        memset(m_propbits, 0, sizeof(m_propbits));
        ioctl(m_fd, EVIOCGPROP(sizeof(m_propbits)), m_propbits);

        m_INPUT_PROP_POINTER = (m_propbits[0] >> INPUT_PROP_POINTER) & 1;
        m_INPUT_PROP_BUTTONPAD = (m_propbits[0] >> INPUT_PROP_BUTTONPAD) & 1;

        m_has_pressure = ((m_bit[EV_ABS][0] >> ABS_PRESSURE) & 1);
        if(m_has_pressure)
            m_pos_max[2] = m_abs_param[ABS_MT_PRESSURE][ABS_PARAM_MAX];

        m_active = true;

        FD_ZERO(&m_rdfs);
        FD_SET(m_fd, &m_rdfs);
        m_thread = new std::thread(&cTouchScreen::loop, this);
    }
    else
    {
        throw log_error(std::string("No Touchscreen dev found at ") + path);
    }
}

void *cTouchScreen::loop(void *obj)
{
    while(reinterpret_cast<cTouchScreen *>(obj)->m_active)
        reinterpret_cast<cTouchScreen *>(obj)->readEv();
    return nullptr;
}

void cTouchScreen::readEv()
{
    FD_SET(m_fd, &m_rdfs);                        //add fd to the set
    select(m_fd + 1, &m_rdfs, NULL, NULL, &m_tv); //watch if fd is readable
    if(FD_ISSET(m_fd, &m_rdfs)) //fd still in set if readable before timeout
    {
        if(read(m_fd, &m_ev, ev_size) < ev_size)
            fprintf(stderr, "Error size when reading\n");
        else
        {
            int i = m_abs_param[ABS_MT_SLOT][ABS_PARAM_VALUE];
            int j = m_ev.code - ABS_MT_POSITION_X;
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
                    m_pressure = m_ev.value;
                    break;
                case ABS_MT_POSITION_X:
                case ABS_MT_POSITION_Y:
                    m_mt_pos[i][j] = m_ev.value;
                    m_mt_pos_rel[i][j] = m_ev.value / m_pos_max[j];
                    if(!m_has_pressure)
                    {
                        m_mt_pos[i][2] = 1;
                        m_mt_pos_rel[i][2] = 1;
                    }
                    break;
                case ABS_MT_PRESSURE:
                    m_mt_pressure[i] = m_ev.value;
                    m_mt_pressure_rel[i] = m_ev.value / m_pressure_max;
                    break;
                case ABS_MT_TRACKING_ID:
                    if(m_ev.value == -1)
                    {
                        for(int k = 0; i < 2; i++)
                        {
                            m_mt_pos[i][k] = -1;
                            m_mt_pos_rel[i][k] = -1;
                        }
                        m_mt_pressure[i] = -1;
                        m_mt_pressure_rel[i] = -1;
                    }
                    break;
                }

                break;
            }
            }
        }
    }
}
