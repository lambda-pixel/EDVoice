# Elite Dangerous Journal Alerts

A lightweight C++ tool that monitors both **Elite Dangerous journal logs** and the **Status.json** file in real time, triggering custom **voice alerts** (via audio playback) when key in-game events occur.

## ⚠️ Disclaimer

Use at your own risk! I am not responsible for any game crashes, FSD explosions, in-game disasters, or even potential computer corruption.

> Seriously, if your PC spontaneously combusts or your Commander’s ship jumps into a star… that’s on you 😜.

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

## 📦 Download and Install

1. Go to the [Releases page](https://github.com/lambda-pixel/EDVoice/releases/latest)
2. Download the latest release zip file, e.g., `EDVoice-v0.1.0.zip`.
3. Extract the zip file to a folder of your choice.
4. Run EDVoice.exe from that folder.

> Optional: you can drag & drop a custom voicepack JSON onto the executable, or pass the path to the JSON as the first argument.

## 🚀 Building from sources
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
- Plugin system for community contributions
- TTS playback support (e.g., ingame chat messages)
- Sample plugin for custom actions (e.g., Philips Hue trigger)
- Improve the audio playback system: allow for queuing multiple messages
