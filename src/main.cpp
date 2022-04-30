#include <iostream>

#include "touchscreen.hpp"

int
main(int argc, char *argv[])
{
    cTouchScreen ts;
    int nb_mt = ts.max_mt();
    std::string clear_string = "\xd" + std::string(nb_mt * 15, ' ') + "\xd";

    for(;;)
    {
        for(int i = 0; i < nb_mt; i++)
            std::cout << "(" << ts.pos(i)[0] << " " << ts.pos(i)[1] << " "
                      << ts.pos(i)[2] << ") ";
        std::cout << std::flush << clear_string;
    }
    return 0;
}
