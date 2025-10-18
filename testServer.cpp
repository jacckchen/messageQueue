#include "Server.h"
int main()
{
     Server server("../config/mq_config.json");
     server.init();
     server.start();
     std::string c;
     std::cin>>c;
     server.stop();
     std::cout << "Server started." << std::endl;
     return 0;
}