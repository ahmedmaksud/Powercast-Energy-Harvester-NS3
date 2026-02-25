# PowerCast Hardware Module - Installation Guide

Complete step-by-step guide for installing and using the PowerCast Hardware Module in NS-3.

## Table of Contents
- [Prerequisites](#prerequisites)
- [Directory Structure](#directory-structure)
- [Installation Steps](#installation-steps)
- [Configuration](#configuration)
- [Building](#building)
- [Running](#running)
- [Verification](#verification)
- [Troubleshooting](#troubleshooting)

## Prerequisites

### Required Software
- **NS-3.44** (or compatible version)
- **GCC/Clang** with C++17 support
- **CMake** 3.10 or higher
- **Python 3.6+** (for visualization scripts)

### Required NS-3 Modules
- `core`
- `network`
- `wifi`
- `energy`
- `mobility`
- `internet`
- `applications`
- `flow-monitor`

### Python Packages (Optional, for visualization)
```bash
pip install pandas matplotlib numpy
```

### Check Your Environment
```bash
# Verify NS-3 installation
ls ~/NS3-project/ns-allinone-3.44/ns-3.44/

# Verify cmake
cmake --version

# Verify python (optional)
python3 --version
```

## Directory Structure

The PowerCast module should be located at:
```
ns-3.44/
└── contrib/
    └── ai/
        └── examples/
            └── powercast_hardware/    ← This directory
                    ├── ph-harvester-hardware.h
                    ├── ph-harvester-hardware.cc
                    ├── ph-deployment-helper.h
                    ├── ph-deployment-helper.cc
                    ├── ph-harvester-demo.cc
                    ├── ph-harvester-config.txt
                    ├── ph-harvester-plot.py
                    ├── CMakeLists.txt
                    ├── README.md
                    └── INSTALL.md (this file)
```

## Installation Steps

### Step 1: Verify Files

Ensure all required files are present:

```bash
cd ~/NS3-project/ns-allinone-3.44/ns-3.44/contrib/ai/examples/powercast_hardware/

# Check core files
ls -la ph-harvester-hardware.*
ls -la ph-deployment-helper.*
ls -la ph-harvester-demo.cc
ls -la ph-harvester-config.txt
ls -la CMakeLists.txt
```

Expected output:
```
ph-harvester-hardware.h
ph-harvester-hardware.cc
ph-deployment-helper.h
ph-deployment-helper.cc
ph-harvester-demo.cc
ph-harvester-config.txt
ph-harvester-plot.py
CMakeLists.txt
```

### Step 2: Verify Parent CMakeLists.txt

The parent directory (`contrib/ai/examples/CMakeLists.txt`) must include this module:

```bash
cd ~/NS3-project/ns-allinone-3.44/ns-3.44/contrib/ai/examples/

# Check if powercast_hardware is referenced
grep -n "powercast_hardware" CMakeLists.txt
```

If not found, add to `contrib/ai/examples/CMakeLists.txt`:
```cmake
add_subdirectory(powercast_hardware)
```

### Step 3: Create Configuration File

If `ph-harvester-config.txt` doesn't exist, create it:

```bash
cd ~/NS3-project/ns-allinone-3.44/ns-3.44/contrib/ai/examples/powercast_hardware/

cat > ph-harvester-config.txt << 'EOF'
# PowerCast Energy Harvester Configuration
# Format: NodeID CapacitorClass VoltageClass
#
# Capacitor Classes: A (50mF), B (2.2mF), C (200mF)
# Voltage Classes: 1 (1.2V), 2 (0.9V), 3 (0.7V)

# Capacitor value definitions (required - values in microfarads)
# CAP_A 50000 μF    # Class A: 50mF super capacitor
# CAP_B 2200 μF     # Class B: 2.2mF electrolytic
# CAP_C 200000 μF   # Class C: 200mF custom large capacity

# Node configurations (NodeID starts from 0 for first STA)
0 A 1    # STA 0: 50mF capacitor, 1.2V threshold
1 B 2    # STA 1: 2.2mF capacitor, 0.9V threshold
2 C 3    # STA 2: 200mF capacitor, 0.7V threshold
3 A 2    # STA 3: 50mF capacitor, 0.9V threshold
EOF
```

## Configuration

### Customize Configuration File

Edit `ph-harvester-config.txt` to match your simulation needs:

```bash
# Open in your preferred editor
nano ph-harvester-config.txt
# or
vim ph-harvester-config.txt
```

**Configuration Options:**

1. **Capacitor Classes:**
   - `A`: 50mF - High capacity, long operation time
   - `B`: 2.2mF - Medium capacity, balanced performance
   - `C`: 200mF - Very high capacity, extended runtime

2. **Voltage Classes:**
   - `1`: 1.2V - High voltage operation (Vmax=1.25V, Vmin=1.02V)
   - `2`: 0.9V - Medium voltage operation (Vmax=0.945V, Vmin=0.9V)
   - `3`: 0.7V - Low voltage operation (Vmax=0.738V, Vmin=0.64V)

3. **Custom Capacitor Values:**

You can override default capacitor values by adding definitions in the config file header:
```
# CAP_A 75000 μF    # Custom 75mF super capacitor
# CAP_B 4700 μF     # Custom 4.7mF electrolytic
# CAP_C 100000 μF   # Custom 100mF capacitor
```

## Building

### Step 1: Navigate to NS-3 Root

```bash
cd ~/NS3-project/ns-allinone-3.44/ns-3.44/
```

### Step 2: Configure Build

```bash
# Configure with examples enabled
./ns3 configure --enable-examples --enable-tests

# Or with optimizations
./ns3 configure --enable-examples --enable-tests --build-profile=optimized
```

Expected output should include:
```
Modules configured to be built:
  ...
  energy                   : enabled
  wifi                     : enabled
  ...
```

### Step 3: Build NS-3

```bash
# Build everything
./ns3 build

# Or build only the PowerCast demo
./ns3 build ph-harvester-demo
```

Expected output:
```
[XXX/XXX] Building CXX object contrib/ai/examples/powercast_hardware/CMakeFiles/ph-harvester-demo.dir/ph-harvester-demo.cc.o
[XXX/XXX] Linking CXX executable ../../../../../../build/contrib/ai/examples/ns3.44-ph-harvester-demo-default
```

### Step 4: Verify Build

```bash
# Check if executable was created
ls -la build/contrib/ai/examples/ns3.44-ph-harvester-demo-default
```

## Running

### Basic Execution

```bash
cd ~/NS3-project/ns-allinone-3.44/ns-3.44/

# Run with default parameters
./ns3 run ph-harvester-demo
```

### With Command-Line Parameters

```bash
# Run 120-second simulation with 8 STAs
./ns3 run "ph-harvester-demo --time=120 --numStas=8"

# Custom power settings
./ns3 run "ph-harvester-demo --apPower=55 --staPower=15"

# Custom data rates and payloads
./ns3 run "ph-harvester-demo --ulRate=6.5 --dlRate=54.0 --ulPayload=64"
```

### Available Command-Line Options

| Parameter | Default | Description |
|-----------|---------|-------------|
| `--time` | 60 | Simulation time in seconds |
| `--numStas` | 4 | Number of STA nodes |
| `--apPower` | 55 | AP transmission power (dBm) |
| `--staPower` | 10 | STA transmission power (dBm) |
| `--ulRate` | 6.5 | Uplink data rate (Mbps) |
| `--dlRate` | 54.0 | Downlink data rate (Mbps) |
| `--ulPayload` | 1 | Uplink payload size (bytes) |
| `--dlPayload` | 2048 | Downlink payload size (bytes) |
| `--configFile` | ph_harvester_config.txt | Configuration file path |

### Example Sessions

**1. Quick Test (30 seconds, 4 STAs):**
```bash
./ns3 run "ph-harvester-demo --time=30 --numStas=4"
```

**2. Extended Simulation (300 seconds, 16 STAs):**
```bash
./ns3 run "ph-harvester-demo --time=300 --numStas=16"
```

**3. High Power Scenario:**
```bash
./ns3 run "ph-harvester-demo --apPower=60 --staPower=20 --time=120"
```

**4. Energy-Efficient Scenario:**
```bash
./ns3 run "ph-harvester-demo --ulPayload=1 --ulRate=6.5 --staPower=10"
```

## Verification

### Check Output

Successful execution should show:

```
📡 PowerCast Energy Harvesting Test - Advanced Energy Model
=============================================================

⚙️  Simulation Configuration:
   • Duration: 60.000 s
   • STAs: 4
   • AP TX Power: 55.00 dBm
   • STA TX Power: 10.00 dBm
   • UL Rate: 6.50 Mbps, Payload: 1 bytes
   • DL Rate: 54.00 Mbps, Payload: 2048 bytes

📁 Configuration File: ph_harvester_config.txt
✅ Configuration loaded successfully
   • Capacitor values: A=50000.00μF, B=2200.00μF, C=200000.00μF
   • Configured 4 STAs from file

... (simulation progress) ...

📊 FINAL STATISTICS (t=60.000s)
================================

[Statistics tables showing energy harvesting results]

✅ Simulation completed successfully!
📄 Detailed CSV log: ph-harvester-demo-log.csv
```

### Verify Output Files

```bash
cd ~/NS3-project/ns-allinone-3.44/ns-3.44/

# Check CSV output
ls -la ph-harvester-demo-log.csv
head -5 ph-harvester-demo-log.csv
```

Expected CSV columns:
```
Timestamp_s,STA_ID,Event_Type,RX_Power_dBm,TX_Power_dBm,...
```

### Visualize Results

```bash
# Run visualization script
python3 ph-harvester-plot.py

# Follow prompts to select STA and generate plots
```

## Troubleshooting

### Build Issues

**Issue 1: Module Not Found**
```
CMake Error: The source directory ".../powercast_hardware" does not exist
```
**Solution:**
```bash
# Verify directory exists
ls -la ~/NS3-project/ns-allinone-3.44/ns-3.44/contrib/ai/examples/powercast_hardware/

# Check parent CMakeLists.txt includes add_subdirectory(powercast_hardware)
```

**Issue 2: Missing Energy Module**
```
fatal error: ns3/energy-harvester.h: No such file or directory
```
**Solution:**
```bash
# Rebuild NS-3 with all modules
cd ~/NS3-project/ns-allinone-3.44/ns-3.44/
./ns3 clean
./ns3 configure --enable-examples --enable-modules=core,network,wifi,energy,mobility,internet,applications
./ns3 build
```

**Issue 3: Compilation Errors**
```
error: 'PowercastEnergyHarvester' was not declared in this scope
```
**Solution:**
```bash
# Verify header includes in source files
grep -n "#include.*ph-harvester" powercast_hardware/*.cc

# Clean and rebuild
./ns3 clean
./ns3 build
```

### Runtime Issues

**Issue 1: Config File Not Found**
```
NS_FATAL_ERROR: Could not open harvester config file: ph_harvester_config.txt
```
**Solution:**
```bash
# Copy config to working directory
cd ~/NS3-project/ns-allinone-3.44/ns-3.44/
cp contrib/ai/examples/powercast_hardware/ph-harvester-config.txt ./
```

**Issue 2: Critical Energy Error**
```
NS_FATAL_ERROR: CRITICAL ERROR: Energy consumption exceeds available energy
```
**Solution:** This indicates a bug in application code. Energy should be checked before consumption:
```cpp
// Correct pattern:
if (harvester->CanSustainTransmission(txPowerDbm, txDuration)) {
    harvester->SetConsumedEnergy(txPowerDbm, txDuration);
    // Send packet
}
```

**Issue 3: No Energy Harvesting**
```
All STAs show 0 available energy throughout simulation
```
**Solution:**
```bash
# 1. Verify PHY trace connections
grep -n "Config::Connect.*PhyRxBegin" powercast_hardware/ph-harvester-demo.cc

# 2. Check received power levels (enable logging)
export NS_LOG="PowercastEnergyHarvester=info"
./ns3 run ph-harvester-demo
```

**Issue 4: Rapid Energy Depletion**
```
STAs run out of energy quickly
```
**Solution:**
- Reduce TX power: `--staPower=10`
- Use smaller payloads: `--ulPayload=1`
- Use larger capacitors: Edit config to use CLASS_C
- Reduce transmission frequency in application code

### Configuration Issues

**Issue 1: Invalid Capacitor Class**
```
Warning: Invalid capacitor class 'D'
```
**Solution:** Only A, B, C are valid. Fix config file:
```bash
nano ph-harvester-config.txt
# Change 'D' to 'A', 'B', or 'C'
```

**Issue 2: Invalid Voltage Class**
```
Warning: Invalid voltage class '4'
```
**Solution:** Only 1, 2, 3 are valid. Fix config file:
```bash
nano ph-harvester-config.txt
# Change '4' to '1', '2', or '3'
```

### Visualization Issues

**Issue 1: Python Package Not Found**
```
ModuleNotFoundError: No module named 'pandas'
```
**Solution:**
```bash
pip install pandas matplotlib numpy
# or
pip3 install pandas matplotlib numpy
```

**Issue 2: CSV File Not Found**
```
FileNotFoundError: ph-harvester-demo-log.csv
```
**Solution:**
```bash
# Run simulation first to generate CSV
./ns3 run ph-harvester-demo

# Then run visualization
python3 ph-harvester-plot.py
```

## Advanced Configuration

### Custom Build Directory

```bash
# Build with custom directory
./ns3 configure --out=custom-build --enable-examples
./ns3 build

# Executable location changes to:
custom-build/contrib/ai/examples/powercast_hardware/ns3.44-ph-harvester-demo-default
```

### Debug Build

```bash
# Configure for debugging
./ns3 configure --enable-examples --build-profile=debug

# Enable detailed logging
export NS_LOG="PowercastEnergyHarvester=level_debug:PowercastEnergyHarvesterHelper=level_debug"
./ns3 run ph-harvester-demo
```

### Integration with Other Modules

To use PowerCast hardware in your own simulation:

```cpp
// In your CMakeLists.txt
build_lib_example(
    NAME your-simulation
    SOURCE_FILES
        your-simulation.cc
        ${CMAKE_SOURCE_DIR}/contrib/ai/examples/powercast_hardware/ph-harvester-hardware.cc
        ${CMAKE_SOURCE_DIR}/contrib/ai/examples/powercast_hardware/ph-deployment-helper.cc
    LIBRARIES_TO_LINK
        ${libcore} ${libnetwork} ${libwifi} ${libenergy}
        ${libmobility} ${libinternet} ${libapplications}
)

// In your source file
#include "contrib/ai/examples/powercast_hardware/ph-harvester-hardware.h"
#include "contrib/ai/examples/powercast_hardware/ph-deployment-helper.h"
```

## Testing

### Verify Installation

Run a quick test:
```bash
cd ~/NS3-project/ns-allinone-3.44/ns-3.44/
./ns3 run "ph-harvester-demo --time=10 --numStas=2"
```

Expected result: Simulation completes in ~10 seconds with CSV output.

### Validate Configuration

Test different configurations:
```bash
# Test all capacitor classes
for cap in A B C; do
    echo "0 $cap 1" > test-config.txt
    ./ns3 run "ph-harvester-demo --time=30 --numStas=1 --configFile=test-config.txt"
done
```

### Check Energy Balance

Verify energy harvesting exceeds consumption:
```bash
./ns3 run "ph-harvester-demo --time=60" | grep "Network Energy"
```

Look for positive net energy values.

## Next Steps

After successful installation:

1. **Read the README.md** for detailed module documentation
2. **Explore the demo code** (`ph-harvester-demo.cc`) for usage examples
3. **Customize configurations** for your research scenarios
4. **Integrate with your simulations** using the provided APIs
5. **Analyze results** using the visualization scripts

## Support

For installation issues:
- Check NS-3 documentation: https://www.nsnam.org/documentation/
- Review NS-3 energy module: https://www.nsnam.org/docs/models/html/energy.html
- Contact: amaks002@ucr.edu

## Useful Commands Reference

```bash
# Build
./ns3 configure --enable-examples
./ns3 build ph-harvester-demo

# Run
./ns3 run ph-harvester-demo
./ns3 run "ph-harvester-demo --help"  # Show all options

# Clean
./ns3 clean
./ns3 clean ph-harvester-demo

# Debug
export NS_LOG="PowercastEnergyHarvester=info"
./ns3 run ph-harvester-demo

# Find files
find . -name "ph-harvester*"
find . -name "ns3.44-ph-harvester-demo-*"

# Check build
ls -la build/contrib/ai/examples/powercast_hardware/
```

---

**Installation Complete!** You're ready to use the PowerCast Hardware Module for realistic RF energy harvesting simulations in NS-3.
