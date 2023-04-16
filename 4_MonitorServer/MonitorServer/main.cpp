#include <iostream>
#include "MonitorServer.h"

int main() {
    std::cout << "Hello MonitorSV!\n";

    MonitorServer_Properties props;
    props.SV_Port = 12300;
    props.CL_Port = 11990;
    props.DB_HostName = "tcp://192.168.0.2:3306/logdb";
    props.DB_UserName = "root";
    props.DB_Password = "1111";

    MonitorServer msv;

    msv.Init(props);
    msv.Start();
    while (true) {
        Sleep(1000);
    }

    return 0;
}