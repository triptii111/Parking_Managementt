#include "API.h"
#include <nlohmann/json.hpp>
#include <ctime>
#include <cmath>
#include <string>

using json = nlohmann::json;

static void cors(httplib::Response& res) {
    res.set_header("Access-Control-Allow-Origin", "*");
    res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    res.set_header("Access-Control-Allow-Headers", "Content-Type");
}

static json vehicleToJson(const Vehicle& v) {
    return json{
        {"vehicleNumber", v.vehicleNumber},
        {"ownerName", v.ownerName},
        {"vehicleType", v.vehicleType},
        {"slotNumber", v.slotNumber},
        {"entryTime", v.getEntryTimeStr()},
        {"exitTime", v.getExitTimeStr()},
        {"isParked", v.isParked}
    };
}

static std::string formatTime(std::time_t t) {
    if (t == 0) return "";
    char buf[32];
    struct tm tm_info;
#ifdef _WIN32
    localtime_s(&tm_info, &t);
#else
    localtime_r(&t, &tm_info);
#endif
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm_info);
    return std::string(buf);
}

static json historyNodeToJson(const HistoryNode* node) {
    return json{
        {"vehicleNumber", node->vehicleNumber},
        {"ownerName", node->ownerName},
        {"vehicleType", node->vehicleType},
        {"slotNumber", node->slotNumber},
        {"entryTime", formatTime(node->entryTime)},
        {"exitTime", formatTime(node->exitTime)}
    };
}

void setupRoutes(httplib::Server& svr, ParkingLot& lot) {

    svr.Options(".*", [](const httplib::Request&, httplib::Response& res) {
        cors(res);
        res.status = 200;
    });

    svr.Get("/api/vehicles", [&lot](const httplib::Request&, httplib::Response& res) {
        cors(res);
        json arr = json::array();
        for (const auto& v : lot.getAllParked()) arr.push_back(vehicleToJson(v));
        res.set_content(arr.dump(), "application/json");
    });

    svr.Get("/api/vehicles/sorted", [&lot](const httplib::Request& req, httplib::Response& res) {
        cors(res);
        std::string by = req.has_param("by") ? req.get_param_value("by") : "time";
        json arr = json::array();
        for (const auto& v : lot.getSortedParked(by)) arr.push_back(vehicleToJson(v));
        res.set_content(arr.dump(), "application/json");
    });

    svr.Post("/api/park", [&lot](const httplib::Request& req, httplib::Response& res) {
        cors(res);
        try {
            auto body   = json::parse(req.body);
            auto vNum   = body.at("vehicleNumber").get<std::string>();
            auto owner  = body.at("owner").get<std::string>();
            auto type   = body.at("type").get<std::string>();
            auto result = lot.parkVehicle(vNum, owner, type);

            if (result == "ALREADY_PARKED") {
                res.status = 400;
                res.set_content(json{{"success", false}, {"message", "Vehicle is already parked"}}.dump(), "application/json");
            } else if (result == "WAITING") {
                res.set_content(json{{"success", true}, {"status", "waiting"}, {"message", "Parking full. Vehicle added to waiting queue"}}.dump(), "application/json");
            } else {
                int slot = std::stoi(result.substr(7));
                res.set_content(json{{"success", true}, {"status", "parked"}, {"slot", slot}, {"message", "Vehicle parked at slot " + std::to_string(slot)}}.dump(), "application/json");
            }
        } catch (const std::exception& e) {
            res.status = 400;
            res.set_content(json{{"success", false}, {"message", std::string("Bad request: ") + e.what()}}.dump(), "application/json");
        }
    });

    svr.Post("/api/exit", [&lot](const httplib::Request& req, httplib::Response& res) {
        cors(res);
        try {
            auto vNum   = json::parse(req.body).at("vehicleNumber").get<std::string>();
            auto result = lot.exitVehicle(vNum);

            if (result == "NOT_FOUND") {
                res.status = 404;
                res.set_content(json{{"success", false}, {"message", "Vehicle not found"}}.dump(), "application/json");
            } else {
                res.set_content(json{{"success", true}, {"message", "Vehicle exited successfully"}}.dump(), "application/json");
            }
        } catch (const std::exception& e) {
            res.status = 400;
            res.set_content(json{{"success", false}, {"message", std::string("Bad request: ") + e.what()}}.dump(), "application/json");
        }
    });

    svr.Get("/api/history", [&lot](const httplib::Request&, httplib::Response& res) {
        cors(res);
        json arr = json::array();
        for (const auto* node : lot.getHistory()) arr.push_back(historyNodeToJson(node));
        res.set_content(arr.dump(), "application/json");
    });

    svr.Get("/api/waiting", [&lot](const httplib::Request&, httplib::Response& res) {
        cors(res);
        json arr = json::array();
        int pos = 1;
        for (const auto& vNum : lot.getWaiting())
            arr.push_back(json{{"position", pos++}, {"vehicleNumber", vNum}});
        res.set_content(arr.dump(), "application/json");
    });

    svr.Get("/api/slots", [&lot](const httplib::Request&, httplib::Response& res) {
        cors(res);
        json arr = json::array();
        const auto& slots = lot.getSlots();
        for (int i = 0; i < (int)slots.size(); i++)
            arr.push_back(json{{"slotNumber", i + 1}, {"occupied", slots[i]}});
        res.set_content(arr.dump(), "application/json");
    });

    svr.Get(R"(/api/search/([^/]+))", [&lot](const httplib::Request& req, httplib::Response& res) {
        cors(res);
        std::string vNum = req.matches[1].str();
        const Vehicle* v = lot.search(vNum);
        if (!v) {
            res.status = 404;
            res.set_content(json{{"success", false}, {"message", "Vehicle not found"}}.dump(), "application/json");
        } else {
            res.set_content(json{{"success", true}, {"vehicle", vehicleToJson(*v)}}.dump(), "application/json");
        }
    });

    svr.Post("/api/undo", [&lot](const httplib::Request&, httplib::Response& res) {
        cors(res);
        auto result = lot.undoLast();
        if (result == "NOTHING_TO_UNDO") {
            res.set_content(json{{"success", false}, {"message", "Nothing to undo"}}.dump(), "application/json");
        } else if (result == "UNDO_PARK") {
            res.set_content(json{{"success", true}, {"message", "Last park undone"}}.dump(), "application/json");
        } else {
            res.set_content(json{{"success", true}, {"message", "Last exit undone"}}.dump(), "application/json");
        }
    });

    svr.Get("/api/stats", [&lot](const httplib::Request&, httplib::Response& res) {
        cors(res);
        double pct = static_cast<double>(lot.getOccupied()) / ParkingLot::TOTAL_SLOTS * 100.0;
        res.set_content(json{
            {"totalSlots", ParkingLot::TOTAL_SLOTS},
            {"occupied", lot.getOccupied()},
            {"free", lot.getFree()},
            {"waiting", lot.getWaitingCount()},
            {"totalToday", lot.getTotalToday()},
            {"occupancyPercent", std::round(pct * 10.0) / 10.0}
        }.dump(), "application/json");
    });
}
