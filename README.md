 Parking Management System

A parking lot management system built with a **C++ backend** and a plain HTML/CSS/JS frontend. The C++ server handles all business logic and serves the frontend directly — no Node.js, no React, no build step.

Features

- Park and exit vehicles with automatic slot allocation
- Waiting queue — vehicles auto-assigned when a slot becomes free
- Undo the last park or exit operation
- Search any vehicle by registration number
- Sort parked vehicles by entry time, slot, or number
- Live slot grid with color-coded availability
- Parking history with search and sort
- Statistics dashboard with occupancy charts

Tech Stack

| Layer    | Technology                          |
|----------|-------------------------------------|
| Backend  | C++17 · cpp-httplib · nlohmann/json |
| Frontend | HTML · CSS · Vanilla JavaScript     |
| Build    | CMake + Ninja                       |

 
API Endpoints

| Method | Endpoint               | Description              |
|--------|------------------------|--------------------------|
| GET    | `/api/stats`           | Occupancy stats          |
| GET    | `/api/vehicles`        | All parked vehicles      |
| GET    | `/api/slots`           | All 50 slot statuses     |
| GET    | `/api/waiting`         | Waiting queue            |
| GET    | `/api/history`         | Parking history          |
| GET    | `/api/search/:number`  | Search vehicle           |
| POST   | `/api/park`            | Park a vehicle           |
| POST   | `/api/exit`            | Exit a vehicle           |
| POST   | `/api/undo`            | Undo last operation      |

## Author

Built by **Tripti Kushwaha**
