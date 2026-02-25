# PowerCast Hardware Module - P21XXCSR-EVB Energy Harvester

## Overview

Production-grade NS-3 implementation of the **PowerCast P21XXCSR-EVB Band 6** RF energy harvester for realistic 2.4GHz WiFi energy harvesting simulations. This module provides accurate modeling of energy harvesting hardware with comprehensive safety mechanisms and flexible deployment configurations.

**Author:** Ahmed Maksud (amaks002@ucr.edu)  
**Affiliation:** SHINE Lab, Texas State University  
**PI:** Marcelo Menezes De Carvalho  
**License:** GPL v2

## Quick Start

### 1. Deploy to NS-3

```bash
cd ~/NS3-project/Powercast-Energy-Harvester-NS3-main
./deploy-to-ns3.sh
```

### 2. Rebuild NS-3

```bash
cd ~/NS3-project/ns-allinone-3.44/ns-3.44
./ns3 build
```

### 3. Activate Virtual Environment

```bash
source ~/NS3-project/EHRL/bin/activate
```

### 4. Run the Harvester Demo

```bash
cd ~/NS3-project/ns-allinone-3.44/ns-3.44
./ns3 run ph-harvester-demo
```

### 5. Generate the Plot

```bash
cd ~/NS3-project/ns-allinone-3.44/ns-3.44/contrib/ai/examples/powercast_hardware
python3 ph-harvester-plot.py
```

## Key Features

### Hardware Modeling
- ✅ **P21XXCSR-EVB Band 6 Specifications**: 2.4 GHz (2400-2500 MHz) operation
- ✅ **Realistic Efficiency Curve**: Based on actual datasheet characterization
- ✅ **Input Power Range**: -40 to +15 dBm with automatic clamping
- ✅ **Fixed Efficiencies**: 90% boost converter, 90% PA from datasheet
- ✅ **Advanced Energy Model**: Three-tier system (nominal/sunk/available energy)

### Safety Mechanisms
- ✅ **Input Power Clamping**: Prevents hardware damage at +15 dBm maximum
- ✅ **Deep Discharge Protection**: DEEP_RATIO (50% of Vmin) prevents over-discharge
- ✅ **Overshoot Protection**: SHOOT_RATIO (150% of Vmax) limits maximum voltage
- ✅ **Pre-transmission Validation**: `CanSustainTransmission()` prevents unrealistic energy states
- ✅ **Critical Error Detection**: NS_FATAL_ERROR for impossible energy consumption

### Configuration System
- ✅ **Three Capacitor Classes**: Configurable via file (A, B, C)
- ✅ **Three Voltage Classes**: 1.2V, 0.9V, 0.7V operation modes
- ✅ **File-based Configuration**: Large-scale deployment support
- ✅ **Per-node Customization**: Individual harvester configurations
- ✅ **Template-based Deployment**: Predefined configuration patterns

## Hardware Specifications

### P21XXCSR-EVB Band 6 Parameters

| Parameter | Value | Description |
|-----------|-------|-------------|
| **Operating Frequency** | 2450 MHz | Center frequency for 2.4GHz WiFi |
| **Input Power Range** | -40 to +15 dBm | Realistic harvesting range |
| **Boost Efficiency** | 90% | Fixed from datasheet |
| **PA Efficiency** | 90% | Fixed for TX modeling |
| **Maximum Input** | +15 dBm | Hardware protection limit |

### Capacitor Classes (Configurable)

| Class | Default Value | Description | Typical Use |
|-------|---------------|-------------|-------------|
| **CLASS_A** | 50 mF | Super capacitor | High capacity, long operation |
| **CLASS_B** | 2.2 mF | Electrolytic | Medium capacity, balanced |
| **CLASS_C** | 200 mF | Custom large | Very high capacity, extended runtime |

**Note:** Capacitor values are loaded from `ph-harvester-config.txt` - no hardcoded defaults.

### Voltage Classes

| Class | Threshold | Vmax | Vmin | Description |
|-------|-----------|------|------|-------------|
| **CLASS_1** | 1.2V | 1.25V | 1.02V | High voltage operation |
| **CLASS_2** | 0.9V | 0.945V | 0.9V | Medium voltage operation |
| **CLASS_3** | 0.7V | 0.738V | 0.64V | Low voltage operation |

## Efficiency Model

### Realistic 2.4GHz Efficiency Curve

The module implements a realistic efficiency curve based on P21XXCSR-EVB characterization:

| Power Range | Efficiency | Behavior |
|-------------|------------|----------|
| < -40 dBm | 0% | Below sensitivity |
| -40 to -30 dBm | 0-10% | Sensitivity threshold |
| -30 to -10 dBm | 10-40% | Active harvesting (typical WiFi) |
| -10 to 0 dBm | 40-70% | High power region |
| 0 to +10 dBm | 70-85% | Peak efficiency approach |
| +10 to +15 dBm | 85% | Maximum saturation |
| > +15 dBm | Clamped | Hardware protection |

## Core Components

### 1. PowercastEnergyHarvester Class

**File:** `ph-harvester-hardware.h/cc`

Main energy harvester implementation with P21XXCSR-EVB specifications.

**Key Methods:**
```cpp
// Energy management
void SetHarvestedEnergy(double rxPowerDbm, Time duration);
void SetConsumedEnergy(double txPowerDbm, Time duration);

// Configuration
void Configure(CapacitorClass capClass, VoltageClass voltClass);

// Status checking
double GetAvailableEnergy() const;
double GetVcap() const;
bool IsOutputEnabled() const;
bool CanSustainTransmission(double txPowerDbm, Time duration) const;

// Statistics
double GetTotalHarvestedEnergy() const;
double GetTotalConsumedEnergy() const;
```

### 2. PowercastEnergyHarvesterHelper Class

**File:** `ph-deployment-helper.h/cc`

Advanced helper for large-scale deployment with file-based configuration.

**Key Features:**
- File-based configuration loading
- Template-based deployment patterns
- Per-node customization
- Automatic energy source management
- Statistics and validation

**Key Methods:**
```cpp
// Configuration
bool LoadConfigurationFromFile(const std::string& filename);
void SetDefaultTemplate(DeploymentTemplate templateType);
void ConfigureNode(uint32_t nodeId, CapacitorClass cap, VoltageClass volt);

// Installation
EnergyHarvesterContainer Install(EnergySourceContainer sources);

// Utilities
std::string GetConfigurationStatistics() const;
bool ExportConfigurationToFile(const std::string& filename) const;
```

### 3. Demo Application

**File:** `ph-harvester-demo.cc`

Comprehensive demonstration of energy harvesting in WiFi networks.

**Features:**
- Multiple STAs with different configurations
- PHY-layer RX signal monitoring
- Real-time energy statistics
- CSV data export for analysis
- Safety mechanism validation

## Configuration File Format

### ph-harvester-config.txt

```
# PowerCast Energy Harvester Configuration
# Format: NodeID CapacitorClass VoltageClass
#
# Capacitor Classes: A, B, C (values defined below)
# Voltage Classes: 1 (1.2V), 2 (0.9V), 3 (0.7V)

# Capacitor value definitions (required)
# CAP_A 50000 μF    # Class A: 50mF super capacitor
# CAP_B 2200 μF     # Class B: 2.2mF electrolytic
# CAP_C 200000 μF   # Class C: 200mF custom

# Node configurations
0 A 1    # Node 0: 50mF capacitor, 1.2V threshold
1 B 2    # Node 1: 2.2mF capacitor, 0.9V threshold
2 C 3    # Node 2: 200mF capacitor, 0.7V threshold
3 A 2    # Node 3: 50mF capacitor, 0.9V threshold
```

## Usage Example

### Basic WiFi Network with Energy Harvesting

```cpp
#include "ph-harvester-hardware.h"
#include "ph-deployment-helper.h"

// Create nodes
NodeContainer apNode, staNodes;
apNode.Create(1);
staNodes.Create(4);

// Setup WiFi (standard NS-3 configuration)
WifiHelper wifi;
wifi.SetStandard(WIFI_STANDARD_80211n);
// ... configure WiFi ...

// Setup energy system
BasicEnergySourceHelper energySourceHelper;
energySourceHelper.Set("BasicEnergySourceInitialEnergyJ", DoubleValue(100.0));
EnergySourceContainer energySources = energySourceHelper.Install(allNodes);

// Load capacitor values and configure harvesters
PowercastEnergyHarvesterHelper harvesterHelper;
if (!harvesterHelper.LoadConfigurationFromFile("ph-harvester-config.txt")) {
    NS_FATAL_ERROR("Failed to load configuration file!");
}

// Install harvesters on STAs
EnergySourceContainer staEnergySources;
for (uint32_t i = 1; i <= 4; ++i) {
    staEnergySources.Add(energySources.Get(i));
}
EnergyHarvesterContainer harvesters = harvesterHelper.Install(staEnergySources);

// Connect PHY traces for energy harvesting
Config::Connect("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/$ns3::WifiPhy/PhyRxBegin",
                MakeCallback(&OnPhyRxBegin));

// Run simulation
Simulator::Run();
```

## Data Output and Analysis

### CSV Output Format

The demo generates `ph-harvester-demo-log.csv` with columns:

| Column | Description |
|--------|-------------|
| Timestamp_s | Simulation time |
| STA_ID | Station identifier |
| Event_Type | RX, TX, TX_BLOCKED |
| RX_Power_dBm | Received power |
| TX_Power_dBm | Transmitted power |
| Harvested_Energy_J | Energy harvested |
| Consumed_Energy_J | Energy consumed |
| Available_Energy_J | Current available energy |
| Vcap_V | Capacitor voltage |
| Output_Status | Output enabled (0/1) |
| Efficiency | Harvesting efficiency |

### Visualization

Use `ph-harvester-plot.py` for data visualization:

```bash
# Interactive analysis
python3.11 ph-harvester-plot.py

# The script will:
# - Load CSV data
# - Show available STAs
# - Prompt for STA selection
# - Generate energy/voltage plots
# - Display statistics
```

## Advanced Features

### Energy State Validation

```cpp
// Check before transmission
Ptr<PowercastEnergyHarvester> harvester = ...;
Time txDuration = MicroSeconds(200);
double txPowerDbm = 15.0;

if (harvester->CanSustainTransmission(txPowerDbm, txDuration)) {
    // Safe to transmit
    harvester->SetConsumedEnergy(txPowerDbm, txDuration);
    // Send packet
} else {
    // Block transmission - insufficient energy
    NS_LOG_WARN("Transmission blocked due to insufficient energy");
}
```

### Real-time Monitoring

```cpp
// Get current status
double availableEnergy = harvester->GetAvailableEnergy();
double voltage = harvester->GetVcap();
bool canTransmit = harvester->IsOutputEnabled();

std::cout << "Available Energy: " << availableEnergy << " J\n";
std::cout << "Capacitor Voltage: " << voltage << " V\n";
std::cout << "Can Transmit: " << (canTransmit ? "Yes" : "No") << "\n";
```

## Research Applications

This module is suitable for research in:

- ✅ **RF Energy Harvesting**: Realistic power beacon and ambient harvesting
- ✅ **Energy-constrained Networks**: IoT and sensor network simulations
- ✅ **Power Management**: Adaptive transmission strategies
- ✅ **Network Lifetime**: Energy availability and duty cycling
- ✅ **MAC Protocol Design**: Energy-aware medium access
- ✅ **Resource Allocation**: Power and energy optimization

## Performance Notes

### Realistic Scenarios
- **Typical WiFi RX Power**: -30 to -10 dBm → 10-40% efficiency
- **Close Proximity**: -10 to 0 dBm → 40-70% efficiency
- **Power Beacon**: 0 to +15 dBm → 70-85% efficiency

### Energy Balance
- **Harvesting Rate**: Depends on RX power and duration
- **Consumption Rate**: Depends on TX power and duty cycle
- **Net Energy**: Balance determines operational lifetime

### Recommended Settings
- **Small Payloads**: 1-64 bytes for energy efficiency
- **Low TX Power**: 10-15 dBm for energy-constrained operation
- **High Capacity Caps**: CLASS_A or CLASS_C for longer runtime

## Troubleshooting

### Common Issues

**1. Config File Not Found**
```
NS_FATAL_ERROR: Could not open harvester config file
```
Solution: Ensure `ph-harvester-config.txt` is in the working directory or provide absolute path.

**2. Negative Energy States**
```
NS_FATAL_ERROR: CRITICAL ERROR: Energy consumption exceeds available energy
```
Solution: Use `CanSustainTransmission()` before energy consumption calls.

**3. No Energy Harvesting**
```
Available energy remains at 0
```
Solution: Verify PHY trace connections and received power levels (should be > -40 dBm).

**4. Rapid Energy Depletion**
```
Energy drops to 0 quickly
```
Solution: Reduce TX power, increase payload efficiency, or use larger capacitor class.

## File Structure

```

├── ph-harvester-hardware.h         # Main harvester header
├── ph-harvester-hardware.cc        # Harvester implementation
├── ph-deployment-helper.h          # Helper class header
├── ph-deployment-helper.cc         # Helper implementation
├── ph-harvester-demo.cc            # Complete demo application
├── ph-harvester-config.txt         # Configuration file
├── ph-harvester-plot.py            # Visualization script
├── CMakeLists.txt                  # Build configuration
├── README.md                       # This file
└── INSTALL.md                      # Installation guide
```

## References

- **PowerCast Corporation:** P21XXCSR-EVB Datasheet v2.1
- **IEEE 802.11:** Wireless LAN Standard
- **NS-3 Energy Model:** Energy module documentation
- **Research Paper:** [Add your publication here]

## Citation

If you use this module in your research, please cite:

```bibtex
@misc{maksud2024powercast,
  author = {Maksud, Ahmed and De Carvalho, Marcelo Menezes},
  title = {PowerCast Hardware Module for NS-3},
  year = {2024},
  institution = {SHINE Lab, Texas State University},
  url = {https://github.com/YOUR_USERNAME/ns3-powercast-hardware}
}
```

## Support

For questions, issues, or contributions:
- **Email:** amaks002@ucr.edu
- **GitHub Issues:** [Repository issues page]
- **Documentation:** See INSTALL.md for setup instructions

## License

This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License version 2 as published by the Free Software Foundation.

---

**PowerCast Hardware Module** - Realistic RF Energy Harvesting for NS-3  
Built with ❤️ at SHINE Lab, Texas State University
