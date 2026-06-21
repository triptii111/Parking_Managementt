# Parking Management System

A parking lot management system built with a **C++ backend** and a plain HTML/CSS/JS frontend. The C++ server handles all business logic and serves the frontend directly — no Node.js, no React, no build step.

## Features

- Park and exit vehicles with automatic slot allocation
- Waiting queue — vehicles auto-assigned when a slot becomes free
- Undo the last park or exit operation
- Search any vehicle by registration number
- Sort parked vehicles by entry time, slot, or number
- Live slot grid with color-coded availability
- Parking history with search and sort
- Statistics dashboard with occupancy charts

## Tech Stack

| Layer    | Technology                          |
|----------|-------------------------------------|
| Backend  | C++17 · cpp-httplib · nlohmann/json |
| Frontend | HTML · CSS · Vanilla JavaScript     |
| Build    | CMake + Ninja                       |
| Deploy   | Docker (Railway / Render)           |

## Project Structure

```text
├── backend/
│   ├── src/
│   │   ├── server.cpp    — HTTP server, all API routes
│   │   └── parking.h     — Vehicle, Queue, LinkedList, Stack, MergeSort, ParkingLot
│   ├── include/          — httplib.h, nlohmann/json.hpp
│   └── CMakeLists.txt
├── frontend/
│   ├── index.html
│   ├── style.css
│   └── app.js
├── Dockerfile
└── README.md
```

## Deploy on Railway (recommended)

1. Push this repo to GitHub
2. Go to [railway.app](https://railway.app) → New Project → Deploy from GitHub repo
3. Select your repo — Railway auto-detects the Dockerfile and deploys
4. Done. Railway provides a public URL automatically

## Deploy on Render

1. Push this repo to GitHub
2. Go to [render.com](https://render.com) → New → Web Service → Connect your repo
3. Set **Environment** to `Docker`
4. Click Deploy — Render builds and runs the container
5. Done. Render provides a public URL automatically

## Run Locally (Windows)

```bash
cd backend
start.bat
```

Server starts at `http://localhost:8080`

## API Endpoints

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
