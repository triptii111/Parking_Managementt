#include "DataStructures.h"

namespace ParkingSort {

// ─── merge two sorted halves ────────────────────────────────
static void doMerge(std::vector<Vehicle>& arr,
                    int left, int mid, int right,
                    const std::string& sortBy) {
    std::vector<Vehicle> L(arr.begin() + left,    arr.begin() + mid + 1);
    std::vector<Vehicle> R(arr.begin() + mid + 1, arr.begin() + right + 1);

    int i = 0, j = 0, k = left;

    while (i < (int)L.size() && j < (int)R.size()) {
        bool pickLeft;
        if (sortBy == "slot") {
            pickLeft = L[i].slotNumber <= R[j].slotNumber;
        } else if (sortBy == "vehicle") {
            pickLeft = L[i].vehicleNumber <= R[j].vehicleNumber;
        } else {
            // default: sort by entry time
            pickLeft = L[i].entryTime <= R[j].entryTime;
        }
        arr[k++] = pickLeft ? L[i++] : R[j++];
    }
    while (i < (int)L.size()) arr[k++] = L[i++];
    while (j < (int)R.size()) arr[k++] = R[j++];
}

// ─── recursive merge sort ───────────────────────────────────
static void doMergeSort(std::vector<Vehicle>& arr,
                         int left, int right,
                         const std::string& sortBy) {
    if (left >= right) return;
    int mid = left + (right - left) / 2;
    doMergeSort(arr, left,    mid,   sortBy);
    doMergeSort(arr, mid + 1, right, sortBy);
    doMerge(arr, left, mid, right, sortBy);
}

// ─── public entry point ─────────────────────────────────────
void sortVehicles(std::vector<Vehicle>& arr, const std::string& sortBy) {
    if (arr.size() <= 1) return;
    doMergeSort(arr, 0, static_cast<int>(arr.size()) - 1, sortBy);
}

} // namespace ParkingSort
