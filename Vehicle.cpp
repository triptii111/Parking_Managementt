#include "Vehicle.h"
#include <cstring>

Vehicle::Vehicle()
    : vehicleNumber(""), ownerName(""), vehicleType(""),
      entryTime(0), exitTime(0), slotNumber(-1), isParked(false) {}

Vehicle::Vehicle(const std::string& vNum, const std::string& owner,
                 const std::string& type, int slot)
    : vehicleNumber(vNum), ownerName(owner), vehicleType(type),
      entryTime(std::time(nullptr)), exitTime(0),
      slotNumber(slot), isParked(true) {}

// ─── helpers ───────────────────────────────────────────────
static std::string timeToStr(std::time_t t) {
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

std::string Vehicle::getEntryTimeStr() const { return timeToStr(entryTime); }
std::string Vehicle::getExitTimeStr()  const { return timeToStr(exitTime);  }
