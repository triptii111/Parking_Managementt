#include <iostream>
#include <fstream>
#include <cmath>
#include <cstdlib>
#include <httplib.h>
#include <nlohmann/json.hpp>
#include "parking.h"

using json = nlohmann::json;

static void cors(httplib::Response& res) {
    res.set_header("Access-Control-Allow-Origin", "*");
    res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    res.set_header("Access-Control-Allow-Headers", "Content-Type");
}

static json vehicleToJson(const Vehicle& v) {
    return json{
        {"vehicleNumber", v.vehicleNumber},
        {"ownerName",     v.ownerName},
        {"vehicleType",   v.vehicleType},
        {"slotNumber",    v.slotNumber},
        {"entryTime",     v.getEntryTimeStr()},
        {"exitTime",      v.getExitTimeStr()},
        {"isParked",      v.isParked}
    };
}

static json historyToJson(const HistoryNode* n) {
    auto fmt = [](std::time_t t) -> std::string {
        if (t == 0) return "";
        char buf[32]; struct tm tm_info;
#ifdef _WIN32
        localtime_s(&tm_info, &t);
#else
        localtime_r(&t, &tm_info);
#endif
        strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm_info);
        return std::string(buf);
    };
    return json{
        {"vehicleNumber", n->vehicleNumber},
        {"ownerName",     n->ownerName},
        {"vehicleType",   n->vehicleType},
        {"slotNumber",    n->slotNumber},
        {"entryTime",     fmt(n->entryTime)},
        {"exitTime",      fmt(n->exitTime)}
    };
}

int main() {
    ParkingLot lot;
    httplib::Server svr;
    std::string frontendPath = "../frontend";

    svr.Options(".*", [](const httplib::Request&, httplib::Response& res) {
        cors(res); res.status = 200;
    });

    svr.Get("/api/vehicles", [&](const httplib::Request&, httplib::Response& res) {
        cors(res);
        json arr = json::array();
        for (const auto& v : lot.getAllParked()) arr.push_back(vehicleToJson(v));
        res.set_content(arr.dump(), "application/json");
    });

    svr.Get("/api/vehicles/sorted", [&](const httplib::Request& req, httplib::Response& res) {
        cors(res);
        std::string by = req.has_param("by") ? req.get_param_value("by") : "time";
        json arr = json::array();
        for (const auto& v : lot.getSortedParked(by)) arr.push_back(vehicleToJson(v));
        res.set_content(arr.dump(), "application/json");
    });

    svr.Post("/api/park", [&](const httplib::Request& req, httplib::Response& res) {
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
                res.set_content(json{{"success", true}, {"status", "waiting"}, {"message", "Parking full. Added to waiting queue"}}.dump(), "application/json");
            } else {
                int slot = std::stoi(result.substr(7));
                res.set_content(json{{"success", true}, {"status", "parked"}, {"slot", slot}, {"message", "Parked at slot " + std::to_string(slot)}}.dump(), "application/json");
            }
        } catch (const std::exception& e) {
            res.status = 400;
            res.set_content(json{{"success", false}, {"message", std::string("Bad request: ") + e.what()}}.dump(), "application/json");
        }
    });

    svr.Post("/api/exit", [&](const httplib::Request& req, httplib::Response& res) {
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

    svr.Get("/api/history", [&](const httplib::Request&, httplib::Response& res) {
        cors(res);
        json arr = json::array();
        for (const auto* node : lot.getHistory()) arr.push_back(historyToJson(node));
        res.set_content(arr.dump(), "application/json");
    });

    svr.Get("/api/waiting", [&](const httplib::Request&, httplib::Response& res) {
        cors(res);
        json arr = json::array();
        int pos = 1;
        for (const auto& vNum : lot.getWaiting())
            arr.push_back(json{{"position", pos++}, {"vehicleNumber", vNum}});
        res.set_content(arr.dump(), "application/json");
    });

    svr.Get("/api/slots", [&](const httplib::Request&, httplib::Response& res) {
        cors(res);
        json arr = json::array();
        const auto& slots = lot.getSlots();
        for (int i = 0; i < (int)slots.size(); i++)
            arr.push_back(json{{"slotNumber", i + 1}, {"occupied", slots[i]}});
        res.set_content(arr.dump(), "application/json");
    });

    svr.Get(R"(/api/search/([^/]+))", [&](const httplib::Request& req, httplib::Response& res) {
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

    svr.Post("/api/undo", [&](const httplib::Request&, httplib::Response& res) {
        cors(res);
        auto result = lot.undoLast();
        bool ok = (result != "NOTHING_TO_UNDO");
        std::string msg = (result == "UNDO_PARK") ? "Last park undone"
                        : (result == "UNDO_EXIT") ? "Last exit undone"
                                                  : "Nothing to undo";
        res.set_content(json{{"success", ok}, {"message", msg}}.dump(), "application/json");
    });

    svr.Get("/api/stats", [&](const httplib::Request&, httplib::Response& res) {
        cors(res);
        double pct = static_cast<double>(lot.getOccupied()) / ParkingLot::TOTAL_SLOTS * 100.0;
        res.set_content(json{
            {"totalSlots",       ParkingLot::TOTAL_SLOTS},
            {"occupied",         lot.getOccupied()},
            {"free",             lot.getFree()},
            {"waiting",          lot.getWaitingCount()},
            {"totalToday",       lot.getTotalToday()},
            {"occupancyPercent", std::round(pct * 10.0) / 10.0}
        }.dump(), "application/json");
    });

    if (!svr.set_mount_point("/", frontendPath)) {
        std::cerr << "Warning: could not mount " << frontendPath << "\n";
    }

    svr.set_error_handler([&frontendPath](const httplib::Request& req, httplib::Response& res) {
        if (res.status == 404 && req.path.rfind("/api/", 0) != 0) {
            std::ifstream f(frontendPath + "/index.html");
            if (f) {
                std::string body((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
                res.status = 200;
                res.set_content(body, "text/html");
            }
        }
    });

    int port = 8080;
    const char* portEnv = std::getenv("PORT");
    if (portEnv) port = std::atoi(portEnv);

    std::cout << "Server running at http://0.0.0.0:" << port << "\n";

    if (!svr.listen("0.0.0.0", port)) {
        std::cerr << "Failed to start server on port " << port << "\n";
        return 1;
    }
    return 0;
}
