# rpi-pico-uart-led-control

> **Multi-client UART-based GPIO control system** for Raspberry Pi Pico, with persistent flash state and automatic UART connection detection.

This project enables a central **Pico-based server** to detect and communicate with multiple **Pico-based clients** over UART. Clients expose GPIOs that can be remotely controlled (ON/OFF/TOGGLE) by the server via UART messages. The server saves the state of all connected devices in internal Flash memory, allowing full restoration after power cycles.

---

## 🧠 System Overview

- Server auto-detects all valid TX/RX pin combinations via UART handshake.
- Each client listens for GPIO control commands and applies them.
- Server sends state updates and stores them in persistent Flash.

---

## ✨ Features

- 🔌 Automatic UART handshake
- 📶 Scans all UART0 & UART1 pin pairs (up to 5 clients)
- 💡 Remote GPIO control
- 🔁 Server tracks all client states, supports:
  - ✅ ON / OFF
  - 🔁 TOGGLE (planned)
  - 💾 SAVE / LOAD running config
  - 📁 3 PRESET configurations per client (planned)
- 💾 Persistent flash memory with CRC32 protection
- 🧪 Menu-based USB CLI interface for live control

---

## 🧰 Requirements

- Raspberry Pi Pico boards (1 server, 1+ clients)
- UART connection wires (TX/RX)
- [Pico SDK](https://github.com/raspberrypi/pico-sdk)
- CMake + Arm GCC toolchain

---

## ⚙️ UART Protocol

### Handshake

```
Client → Server : "Requesting Connection-[TX,RX]"
Server → Client : "[TX,RX]"
Client → Server : "[Connection Accepted]"
```

### Command Format

```
Server → Client : "[gpio_number,value]"
Example: "[2,1]" → turn GPIO 2 ON
```

(Planned: additional messages for TOGGLE, SAVE, LOAD, etc.)

---

## 🛠️ How to Build (Multiple Environments)

This project supports multiple build systems using CMake. The `-DPICO_BOARD=pico` flag is used to select the board version — in this case, `pico` is used for all examples below. The `CMakeLists.txt` files are configured so that the build outputs flash files for both the server and the client separately, each in its own folder.

Below are instructions for common configurations:

### 🔧 Build with **MinGW Makefiles** (recommended for Windows)

```bash
mkdir build
cd build
# Generate Makefiles using MinGW generator
cmake -G "MinGW Makefiles" .. -DPICO_BOARD=pico
# Build
mingw32-make
```

In this example we used only one type of board. In case there are multiple types of pico boards, the following process is suggested:
```bash
mkdir build
cd build
# Create build folder for pico board type #1
mkdir build_pico
cd build_pico
cmake -G "MinGW Makefiles" ../.. -DPICO_BOARD=pico
mingw32-make
# Wait for the build to finish
cd ..
# Create build folder for pico board type #2, pico2_w is used in this example
mkdir build_pico2_w
cd build_pico2_w
cmake -G "MinGW Makefiles" ../.. -DPICO_BOARD=pico2_w
mingw32-make
```
Each board type now has its own build folder, containing both the client and server firmware ready to be flashed.  
📝 Make sure `mingw32-make` and `arm-none-eabi-gcc` are in your `PATH`.

---

### 🧱 Build with **Ninja** (cross-platform, fast builds)

```bash
mkdir build
cd build
cmake -G "Ninja" .. -DPICO_BOARD=pico
ninja
```

---

### 🐧 Build with **Unix Makefiles** (Linux/macOS)

```bash
mkdir build
cd build
cmake -G "Unix Makefiles" .. -DPICO_BOARD=pico
make
```

---

### 🪟 Build with **Visual Studio 2022** (Windows)

```powershell
mkdir build
cd build
cmake -G "Visual Studio 17 2022" .. -DPICO_BOARD=pico
# Optional: build from terminal
cmake --build . --config Debug
```

---

## 🚀 Flashing to Raspberry Pi Pico

### 🖱️ Option 1: Drag-and-drop (BOOTSEL mode)

1. Hold `BOOTSEL` while plugging in the Pico via USB.
2. It will mount as a drive.
3. Drag the `.uf2` file onto it.

### 🔧 Option 2: Use `picotool` (if installed)

```bash
picotool load firmware.uf2
```

Requires:
- `picotool` installed
- Pico connected normally (not in BOOTSEL)
- UART or USB serial enabled

---

## 🧪 How It Works

1. **Power on the server** — it begins scanning UART pin pairs for client requests.
2. **Power on a client** — it starts scanning and broadcasting handshake messages.
3. Once handshake succeeds:
   - Server stores the active connection.
   - Server can start sending `[gpio_number, value]` messages to control client.
4. Server persists all client states to internal Flash.
5. Upon reboot, saved states are restored.

---

## 🔧 Configuration

See `types.c` and `config.h` to customize:

- UART pin pairs per UART instance
- Timeout settings
- Max number of GPIOs per client
- Flash memory offset and sector size

---

## 📈 Roadmap

- [x] Multi-client support
- [x] Flash-based state persistence with CRC
- [ ] TOGGLE command
- [ ] SAVE/LOAD configuration presets (3 per client)
- [ ] UART CRC validation
- [ ] Timeouts and retry logic
- [ ] More flexible GPIO mapping

---

## 🧠 Design Considerations

- Safe Flash writes are executed via `__not_in_flash_func`.
- Handshake timeout is tunable per side.
- Clients reset UART pins to GPIO after receiving state.

---

## 📜 License

Licensed under the [BSD-3-Clause License](https://opensource.org/licenses/BSD-3-Clause).

---

## 👤 Author

**Dragos Hategan**  
