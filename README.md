# Elite Dangerous Journal Alerts

A lightweight C++ tool that monitors both **Elite Dangerous journal logs** and the **Status.json** file in real time, triggering custom **voice alerts** (via audio playback) when key in-game events occur.

## Features
- Real-time monitoring of:
  - `Journal.*.log` files
  - `Status.json`  
- Detection of important in-game events such as:
  - Docking granted / denied
  - FSD jumps
  - Landing gear status
  - And more...
- Playback of custom audio alerts
- Extensible design: add new events and alerts easily

## 🚀 Getting Started
1. Clone the repository:
```bash
git clone https://github.com/<your-username>/<your-repo>.git
cd <your-repo>
```

2. Build with CMake (requires a C++17 compiler on Windows):
```bash
mkdir build && cd build
cmake ..
cmake --build . --config Release
```

3. Run the tool while playing Elite Dangerous.
By default, the game logs are located in: `%USERPROFILE%\Saved Games\Frontier Developments\Elite Dangerous\`

## 🛠 Roadmap
- Add configuration file for custom audio mapping
- GUI for event selection and testing alerts
- More flexible trigger system to allow other type of actions (e.g., Philips Hue trigger)
