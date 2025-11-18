#include "api_data.h"
#include <json/json.h>

using namespace api;

namespace api
{
    void data::postData(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback)
    {
        auto jsonStr = req->getJsonObject();
        if (!jsonStr)
        {
            Json::Value status;
            status["Error"] = "Invalid JSON";
            HttpResponsePtr resp = HttpResponse::newHttpJsonResponse(status);
            callback(resp);
            return;
        }

        /* debug: Check datatype of value
        if (!(*jsonStr)["sensor_id"].isInt() || !(*jsonStr)["float"].isNumeric())
        {
            Json::Value status;
            status["Error"] = "sensor_id must be interger and light must be number";
            HttpResponsePtr resp = HttpResponse::newHttpJsonResponse(status);
            callback(resp);
            return;
        }
        */

        // LOG_INFO << "Received JSON: " << jsonStr->toStyledString(); //debug

        // std::cout << "POST JSON: " << jsonStr << std::endl; // debug: check JSON ESP32 gửi

        int sensor_id = (*jsonStr)["sensor_id"].asInt();
        float light = (*jsonStr)["light"].asFloat();

        auto client = drogon::app().getDbClient("default");

        // std::cout << "DB CLIENT PTR = " << client.get() << std::endl; // debug

        std::string SQL_insert = "INSERT INTO bh1750 (sensor_id, light) VALUES (?, ?)";
        client->execSqlAsync(
            SQL_insert,
            [=](const drogon::orm::Result &result)
            {
                std::cout << result.affectedRows() << " inserted!" << std::endl;
                Json::Value status;
                status["Status"] = "Received";
                // Debug: check data by client
                status["sensor_id"] = sensor_id;
                status["light"] = light;
                // std::cout << sensor_id << std::endl; // debug
                // spdlog::info("Light: {}", sensor_id);
                auto response = HttpResponse::newHttpJsonResponse(status);
                callback(response);
            },
            [=](const drogon::orm::DrogonDbException &e)
            {
                std::cerr << "error: " << e.base().what() << std::endl;
                Json::Value status;
                status["Error"] = e.base().what();
                auto response = HttpResponse::newHttpJsonResponse(status);
                callback(response);
            },
            sensor_id,
            light);

        // test: curl -X POST http://localhost:5000/api/data/postData -H "Content-Type: application/json" -d "{\"sensor_id\":105,\"light\":192}"
    }

    void data::getData(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback)
    {
        // Get Database client
        auto client = drogon::app().getDbClient("default");

        // Queries execute
        std::string SQL_select = "SELECT sensor_id, date, time, light FROM bh1750 ORDER BY date DESC, time DESC LIMIT 10"; // Lấy 10 giá trị mới nhất
        client->execSqlAsync(
            SQL_select,
            [=](const drogon::orm::Result &result)
            {
                std::cout << result.size() << " rows selected" << std::endl; // Log trạng thái đã nhận GET request

                std::vector<std::map<std::string, std::string>> dataList;
                for (auto row : result)
                {
                    std::map<std::string, std::string> rowData;
                    rowData["sensor_id"] = std::to_string(row["sensor_id"].as<int>());
                    rowData["date"] = row["date"].as<std::string>();
                    rowData["time"] = row["time"].as<std::string>();
                    rowData["light"] = std::to_string(row["light"].as<float>());
                    dataList.push_back(rowData);
                }

                HttpViewData viewData;
                viewData.insert("data", dataList);

                auto resp = drogon::HttpResponse::newHttpViewResponse("data.csp", viewData);
                callback(resp);
            },

            [=](const drogon::orm::DrogonDbException &e)
            {
                std::cerr << "error: " << e.base().what() << std::endl;
                Json::Value status;
                status["Error"] = e.base().what();
                auto response = HttpResponse::newHttpJsonResponse(status);
                callback(response);
            });
    }
}
