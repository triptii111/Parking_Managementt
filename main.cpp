#include <iostream>
#include <fstream>
#include <httplib.h>
#include "ParkingLot.h"
#include "API.h"

int main() {
    ParkingLot lot;
    httplib::Server svr;

    setupRoutes(svr, lot);

    std::string distPath = "../frontend";

    if (!svr.set_mount_point("/", distPath)) {
        std::cerr << "Warning: could not mount " << distPath << " (run from backend/ directory)\n";
    }

    svr.set_error_handler([&distPath](const httplib::Request& req, httplib::Response& res) {
        if (res.status == 404 && req.path.rfind("/api/", 0) != 0) {
            std::ifstream f(distPath + "/index.html");
            if (f) {
                std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
                res.status = 200;
                res.set_content(content, "text/html");
            }
        }
    });

    std::cout << "http://localhost:8080\n";

    if (!svr.listen("0.0.0.0", 8080)) {
        std::cerr << "Failed to start server on port 8080\n";
        return 1;
    }
    return 0;
}
