#include <drogon/drogon.h>

int main()
{
    // DB Initialize
    /*
    drogon::orm::MysqlConfig MySQL_Config = {
        .host = "localhost",
        .port = 3306,
        .databaseName = "sensor",
        .username = "hphuc15",
        .password = "hongphucv9@",
        .connectionNumber = 1,
        .name = "default"
    };

    drogon::app().addDbClient(MySQL_Config);
    */

    // Set HTTP listener address and port
    drogon::app().addListener("0.0.0.0", 5000);
    // Load config file
    drogon::app().loadConfigFile("../config.json");
    // drogon::app().loadConfigFile("../config.yaml");
    // Run HTTP framework,the method will block in the internal event loop
    drogon::app().run();
    return 0;
}
