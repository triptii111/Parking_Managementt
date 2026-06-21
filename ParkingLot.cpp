#include "ParkingLot.h"
#include <algorithm>
#include <stdexcept>

ParkingLot::ParkingLot()
    : slots(TOTAL_SLOTS, false), totalToday(0) {}

// ─── internal helpers ───────────────────────────────────────

int ParkingLot::findFreeSlot() const {
    for (int i = 0; i < TOTAL_SLOTS; i++) {
        if (!slots[i]) return i + 1; // 1-indexed
    }
    return -1;
}

// Assign the next free slot to the front of the waiting queue
void ParkingLot::assignFromQueue() {
    if (waitQueue.isEmpty()) return;
    int slot = findFreeSlot();
    if (slot == -1) return;

    std::string vNum = waitQueue.dequeue();
    auto it = parked.find(vNum);
    if (it != parked.end()) {
        it->second.slotNumber = slot;
        it->second.isParked   = true;
        slots[slot - 1]       = true;
    }
}

// ─── core operations ────────────────────────────────────────

std::string ParkingLot::parkVehicle(const std::string& vNum,
                                     const std::string& owner,
                                     const std::string& type) {
    // O(1) duplicate check via hash map
    auto it = parked.find(vNum);
    if (it != parked.end() && (it->second.isParked || it->second.slotNumber == -1)) {
        return "ALREADY_PARKED";
    }

    ++totalToday;
    int slot = findFreeSlot();

    if (slot == -1) {
        // Lot is full — add to waiting queue
        Vehicle v(vNum, owner, type, -1);
        v.isParked   = false;
        parked[vNum] = v;
        waitQueue.enqueue(vNum);
        undoStack.push(Action(ActionType::PARK, v));
        return "WAITING";
    }

    Vehicle v(vNum, owner, type, slot);
    slots[slot - 1] = true;
    parked[vNum]    = v;
    undoStack.push(Action(ActionType::PARK, v));
    return "PARKED:" + std::to_string(slot);
}

std::string ParkingLot::exitVehicle(const std::string& vNum) {
    auto it = parked.find(vNum);
    if (it == parked.end()) return "NOT_FOUND";

    Vehicle v   = it->second;
    v.exitTime  = std::time(nullptr);
    v.isParked  = false;

    history.addRecord(v);
    undoStack.push(Action(ActionType::EXIT, v));

    if (v.slotNumber > 0) slots[v.slotNumber - 1] = false;
    parked.erase(it);

    assignFromQueue(); // auto-assign freed slot
    return "EXITED";
}

std::string ParkingLot::undoLast() {
    if (undoStack.empty()) return "NOTHING_TO_UNDO";

    Action action = undoStack.top();
    undoStack.pop();

    if (action.type == ActionType::PARK) {
        // Reverse a park: remove the vehicle
        auto it = parked.find(action.vehicle.vehicleNumber);
        if (it != parked.end()) {
            if (it->second.slotNumber > 0) slots[it->second.slotNumber - 1] = false;
            parked.erase(it);
        }
        // Rebuild waiting queue excluding this vehicle (O(n) — unavoidable for mid-queue removal)
        auto items = waitQueue.toVector();
        while (!waitQueue.isEmpty()) waitQueue.dequeue();
        for (const auto& item : items) {
            if (item != action.vehicle.vehicleNumber) waitQueue.enqueue(item);
        }
        totalToday = std::max(0, totalToday - 1);
        return "UNDO_PARK";
    }

    // Reverse an exit: re-park the vehicle
    int slot = findFreeSlot();
    Vehicle v = action.vehicle;
    v.exitTime = 0;

    if (slot == -1) {
        v.slotNumber = -1;
        v.isParked   = false;
        parked[v.vehicleNumber] = v;
        waitQueue.enqueue(v.vehicleNumber);
    } else {
        v.slotNumber            = slot;
        v.isParked              = true;
        slots[slot - 1]         = true;
        parked[v.vehicleNumber] = v;
    }
    return "UNDO_EXIT";
}

// ─── data access ────────────────────────────────────────────

const Vehicle* ParkingLot::search(const std::string& vNum) const {
    auto it = parked.find(vNum);
    return (it == parked.end()) ? nullptr : &it->second;
}

std::vector<Vehicle> ParkingLot::getAllParked() const {
    std::vector<Vehicle> result;
    result.reserve(parked.size());
    for (const auto& [k, v] : parked) result.push_back(v);
    return result;
}

std::vector<Vehicle> ParkingLot::getSortedParked(const std::string& by) const {
    std::vector<Vehicle> result = getAllParked();
    ParkingSort::sortVehicles(result, by);
    return result;
}

std::vector<HistoryNode*> ParkingLot::getHistory()  const { return history.toVector();    }
std::vector<std::string>  ParkingLot::getWaiting()  const { return waitQueue.toVector();  }
const std::vector<bool>&  ParkingLot::getSlots()    const { return slots;                 }

// ─── stats ──────────────────────────────────────────────────

int ParkingLot::getOccupied() const {
    int n = 0;
    for (bool b : slots) if (b) ++n;
    return n;
}
int ParkingLot::getFree()         const { return TOTAL_SLOTS - getOccupied(); }
int ParkingLot::getWaitingCount() const { return waitQueue.getSize();         }
int ParkingLot::getTotalToday()   const { return totalToday;                  }
