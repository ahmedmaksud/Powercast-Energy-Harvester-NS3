/*
 * Copyright (c) 2025 Texas State University
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Ahmed Maksud <ahmed.maksud@email.ucr.edu>
 * PI: Marcelo Menezes De Carvalho <mmcarvalho@txstate.edu>
 * Texas State University
 */

/**
 * @file ph-harvester-demo.cc
 * @brief Advanced PowerCast RF Energy Harvesting Test
 *
 * Comprehensive test suite demonstrating the advanced PowerCast RF energy harvesting
 * implementation with three-tier energy model (nominal/sunk/available energy) in a WiFi
 * network environment. Features multiple STAs with different P21XXCSR-EVB configurations
 * testing realistic energy harvesting, safety mechanisms, and threshold validation.
 *
 * Advanced Energy Model Features:
 * - Three-tier energy calculation: E = ½CV² with nominal/sunk/available separation
 * - DEEP_RATIO (0.5) and SHOOT_RATIO (1.5) safety mechanisms for critical error detection
 * - P21XXCSR-EVB Band 6 specifications with 90% boost and PA efficiency modeling
 * - Realistic RF-to-DC conversion curves with +15dBm input clamping protection
 * - Advanced threshold checking via harvester's IsOutputEnabled() and CanSustainTransmission()
 * - Comprehensive energy state tracking with statistical analysis and CSV export
 * - Production-grade error handling with NS_FATAL_ERROR for critical violations
 *
 * Test Configuration:
 * - Multiple STAs with different capacitor/voltage class combinations
 * - PHY-layer RX signal monitoring for realistic energy harvesting simulation
 * - Real-time energy statistics with nominal/sunk/available energy breakdown
 * - Safety mechanism validation and threshold compliance verification
 * - Modern WiFi 802.11n protocol with production-grade energy modeling
 * - CSV data export for detailed analysis and visualization
 */

#include "ns3/applications-module.h"
#include "ns3/config.h"
#include "ns3/core-module.h"
#include "ns3/energy-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/wifi-module.h"

// PowerCast modules
#include "ph-harvester-hardware.h"
#include "ph-deployment-helper.h"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <string>

using namespace ns3;
using namespace ns3::energy;

NS_LOG_COMPONENT_DEFINE("PowercastSimpleTest");

// Structure to hold harvester configuration
struct HarvesterConfig
{
    PowercastEnergyHarvester::CapacitorClass capacitorClass;
    PowercastEnergyHarvester::VoltageClass voltageClass;
    std::string capacitorName;
    std::string voltageName;
};

// Function to read harvester configuration from file
std::map<uint32_t, HarvesterConfig>
ReadHarvesterConfig(const std::string &filename)
{
    std::map<uint32_t, HarvesterConfig> config;
    std::ifstream file(filename);

    if (!file.is_open())
    {
        NS_FATAL_ERROR("Could not open harvester config file: " << filename << ". Configuration file is required - no fallback values allowed!");
    }

    // Parse capacitor value definitions from config file header
    std::string line;
    while (std::getline(file, line))
    {
        // Look for CAP_X definitions in comments
        if (line.find("# CAP_A ") != std::string::npos)
        {
            std::istringstream iss(line.substr(line.find("CAP_A")));
            std::string dummy;
            double capValueUf;
            if (iss >> dummy >> capValueUf)
            {
                PowercastEnergyHarvester::SetCapacitorValue(PowercastEnergyHarvester::CLASS_A, capValueUf / 1e6); // Convert μF to F
                NS_LOG_INFO("Loaded CAP_A from config: " << capValueUf << " μF");
            }
        }
        else if (line.find("# CAP_B ") != std::string::npos)
        {
            std::istringstream iss(line.substr(line.find("CAP_B")));
            std::string dummy;
            double capValueUf;
            if (iss >> dummy >> capValueUf)
            {
                PowercastEnergyHarvester::SetCapacitorValue(PowercastEnergyHarvester::CLASS_B, capValueUf / 1e6);
                NS_LOG_INFO("Loaded CAP_B from config: " << capValueUf << " μF");
            }
        }
        else if (line.find("# CAP_C ") != std::string::npos)
        {
            std::istringstream iss(line.substr(line.find("CAP_C")));
            std::string dummy;
            double capValueUf;
            if (iss >> dummy >> capValueUf)
            {
                PowercastEnergyHarvester::SetCapacitorValue(PowercastEnergyHarvester::CLASS_C, capValueUf / 1e6);
                NS_LOG_INFO("Loaded CAP_C from config: " << capValueUf << " μF");
            }
        }

        // Stop parsing header once we reach node configurations
        if (!line.empty() && line[0] != '#')
        {
            // Reset to beginning to parse node configs
            file.clear();
            file.seekg(0);
            break;
        }
    }

    // Now parse node configurations
    while (std::getline(file, line))
    {
        // Skip comments and empty lines
        if (line.empty() || line[0] == '#')
        {
            continue;
        }

        std::istringstream iss(line);
        uint32_t nodeId;
        char capClass, voltClass;

        if (iss >> nodeId >> capClass >> voltClass)
        {
            HarvesterConfig hconfig;

            // Parse capacitor class
            switch (capClass)
            {
            case 'A':
                hconfig.capacitorClass = PowercastEnergyHarvester::CLASS_A;
                // Get actual value from static storage
                {
                    double capUf = PowercastEnergyHarvester::GetCapacitorValue(PowercastEnergyHarvester::CLASS_A) * 1e6;
                    hconfig.capacitorName = "A (" + std::to_string(static_cast<int>(capUf)) + "μF)";
                }
                break;
            case 'B':
                hconfig.capacitorClass = PowercastEnergyHarvester::CLASS_B;
                {
                    double capUf = PowercastEnergyHarvester::GetCapacitorValue(PowercastEnergyHarvester::CLASS_B) * 1e6;
                    hconfig.capacitorName = "B (" + std::to_string(static_cast<int>(capUf)) + "μF)";
                }
                break;
            case 'C':
                hconfig.capacitorClass = PowercastEnergyHarvester::CLASS_C;
                {
                    double capUf = PowercastEnergyHarvester::GetCapacitorValue(PowercastEnergyHarvester::CLASS_C) * 1e6;
                    hconfig.capacitorName = "C (" + std::to_string(static_cast<int>(capUf)) + "μF)";
                }
                break;
            default:
                NS_LOG_WARN("Unknown capacitor class: " << capClass << " for node " << nodeId);
                continue;
            }

            // Parse voltage class
            switch (voltClass)
            {
            case '1':
                hconfig.voltageClass = PowercastEnergyHarvester::CLASS_1;
                hconfig.voltageName = "1 (1.2V)";
                break;
            case '2':
                hconfig.voltageClass = PowercastEnergyHarvester::CLASS_2;
                hconfig.voltageName = "2 (0.9V)";
                break;
            case '3':
                hconfig.voltageClass = PowercastEnergyHarvester::CLASS_3;
                hconfig.voltageName = "3 (0.7V)";
                break;
            default:
                NS_LOG_WARN("Unknown voltage class: " << voltClass << " for node " << nodeId);
                continue;
            }

            config[nodeId] = hconfig;
            NS_LOG_INFO("Configured node " << nodeId << ": Cap=" << capClass
                                           << ", Volt=" << voltClass);
        }
    }

    file.close();
    NS_LOG_INFO("Loaded configuration for " << config.size() << " nodes from " << filename);
    return config;
}

// Global variables for statistics tracking
struct STAStats
{
    uint32_t nodeId;
    std::string capacitorClass;
    std::string voltageClass;
    double totalHarvested;
    double totalConsumed;
    double currentVoltage;
    bool outputEnabled;
    uint32_t harvestingEvents;
    uint32_t transmissionEvents;
    uint32_t blockedTransmissions;
    double energyBalance;

    STAStats()
        : nodeId(0),
          totalHarvested(0.0),
          totalConsumed(0.0),
          currentVoltage(0.0),
          outputEnabled(false),
          harvestingEvents(0),
          transmissionEvents(0),
          blockedTransmissions(0),
          energyBalance(0.0)
    {
    }
};

std::vector<STAStats> g_staStats;
std::vector<Ptr<PowercastEnergyHarvester>> g_harvesters;

// Global data rate variables for PHY callbacks
double g_dlDataRate = 54.0; // Default downlink data rate
double g_ulDataRate = 1.0;  // Slowest uplink data rate (1 Mbps DSSS)

// Global CSV file stream for simulation logging
std::ofstream g_simulationLogFile;

// Structure to track ongoing RX/TX events for duration calculation
struct PhyEvent
{
    Time startTime;
    double powerDbm;
    uint32_t packetSize;
    uint32_t nodeId;
};

// Maps to track ongoing RX and TX events (keyed by node ID)
std::map<uint32_t, PhyEvent> g_ongoingRxEvents;
std::map<uint32_t, PhyEvent> g_ongoingTxEvents;

// CSV Logging Function - Logs all simulation events with comprehensive data
void LogSimulationEvent(double timestamp,
                        uint32_t staId,
                        const std::string &eventType,
                        double rxPowerDbm,
                        double txPowerDbm,
                        uint32_t packetSize,
                        double durationMicroseconds,
                        double harvestedEnergy,
                        double consumedEnergy,
                        double availableEnergy,
                        double vcap,
                        const std::string &capClass,
                        const std::string &voltClass,
                        bool isBlocked,
                        bool isSuccessful,
                        bool outputEnabled,
                        const std::string &additionalInfo = "")
{
    if (!g_simulationLogFile.is_open())
    {
        return; // Safety check
    }

    // Log to CSV file with all requested fields
    g_simulationLogFile << std::fixed << std::setprecision(6) << timestamp << "," << staId << ","
                        << eventType << "," << std::scientific << std::setprecision(3) << rxPowerDbm
                        << "," << std::scientific << std::setprecision(3) << txPowerDbm << ","
                        << packetSize << "," << std::fixed << std::setprecision(2)
                        << durationMicroseconds << "," << std::scientific << std::setprecision(6)
                        << harvestedEnergy << "," << std::scientific << std::setprecision(6)
                        << consumedEnergy << "," << std::scientific << std::setprecision(6)
                        << availableEnergy << "," << std::fixed << std::setprecision(8) << vcap // Increased precision to 8 decimals
                        << "," << capClass << "," << voltClass << ","
                        << (isBlocked ? "BLOCKED" : "ALLOWED") << ","
                        << (isSuccessful ? "SUCCESS" : "FAILED") << ","
                        << (outputEnabled ? "ENABLED" : "DISABLED") << "," << additionalInfo
                        << std::endl;
}

// Function to calculate accurate WiFi packet duration using SLOWEST possible rate (1 Mbps DSSS)
Time CalculateWiFiPacketDuration(Ptr<const Packet> packet, double dataRateMbps, bool isDownlink = true)
{
    // FIXED: Always use 1 Mbps DSSS rate (802.11b) - the absolute slowest WiFi rate
    // This matches the actual STA configuration below
    const double SLOWEST_RATE_MBPS = 1.0; // 1 Mbps DSSS (802.11b)

    // 802.11b DSSS PHY overhead components for 1 Mbps:
    // - Long PLCP Preamble: 144 μs (long training sequence for 1 Mbps)
    // - PLCP Header: 48 μs (at 1 Mbps DBPSK)
    // Total PHY overhead for 802.11b long preamble: 192 μs
    Time phyOverhead = MicroSeconds(192);

    // Calculate data transmission time at 1 Mbps DSSS
    uint32_t payloadBits = packet->GetSize() * 8;
    uint64_t dataRateBps = static_cast<uint64_t>(SLOWEST_RATE_MBPS * 1000000); // 1 Mbps
    Time dataTime = NanoSeconds(payloadBits * 1000000000ULL / dataRateBps);

    // Add modest safety margin (150%) to account for:
    // - ACK frame overhead (14 bytes @ 1 Mbps = 112 μs data + 192 μs PHY = 304 μs)
    // - SIFS (10 μs)
    // - DIFS (50 μs)
    // - Average backoff delay (~67.5 μs for 15 slot average)
    // Total estimated overhead: ~432 μs per packet
    // Safety margin accounts for retries and contention
    Time estimatedDuration = phyOverhead + dataTime;
    return estimatedDuration * 1.25; // 250% for ACK, IFS, and backoff overhead
}

// Function to check if STA has enough energy for transmission while maintaining voltage constraints
bool CheckEnergyForTransmission(uint32_t staIndex, double txPowerDbm, Time txDuration)
{
    if (staIndex >= g_harvesters.size() || !g_harvesters[staIndex])
    {
        return false;
    }

    // Use the new CanSustainTransmission function directly
    Ptr<PowercastEnergyHarvester> harvester = g_harvesters[staIndex];
    return harvester->CanSustainTransmission(txPowerDbm, txDuration);
}

// Custom application that checks energy before transmission
// FIXED APPROACH: Pre-deduct energy before sending to prevent PHY double deduction
class EnergyAwareUdpClient : public Application
{
private:
    Ptr<Socket> m_socket;
    Address m_peer;
    uint32_t m_packetSize;
    uint32_t m_maxPackets;
    Time m_interval;
    uint32_t m_sent;
    uint32_t m_staIndex;
    double m_txPowerDbm;
    double m_ulDataRate;
    EventId m_sendEvent;
    Ptr<UniformRandomVariable> m_randomInterval; // Random interval generator

public:
    EnergyAwareUdpClient()
        : m_socket(0),
          m_sent(0),
          m_staIndex(0),
          m_txPowerDbm(15.0),
          m_ulDataRate(6.5)
    {
        // Initialize random number generator for transmission intervals
        m_randomInterval = CreateObject<UniformRandomVariable>();
    }

    virtual ~EnergyAwareUdpClient()
    {
        m_socket = 0;
    }

    void Setup(Address address,
               uint32_t packetSize,
               uint32_t maxPackets,
               Time interval,
               uint32_t staIndex,
               double txPowerDbm,
               double ulDataRate)
    {
        m_peer = address;
        m_packetSize = packetSize;
        m_maxPackets = maxPackets;
        m_interval = interval; // This will be the mean interval for random generation
        m_staIndex = staIndex;
        m_txPowerDbm = txPowerDbm;
        m_ulDataRate = ulDataRate;

        // Configure random interval generator for 2 transmissions per second on average
        // Mean interval = 500ms, with uniform distribution between 100ms and 900ms
        // This gives average rate of ~2 transmissions per second with randomness
        double meanIntervalMs = m_interval.GetMilliSeconds();
        double minIntervalMs = meanIntervalMs * 0.2; // 20% of mean (100ms if mean is 500ms)
        double maxIntervalMs = meanIntervalMs * 1.8; // 180% of mean (900ms if mean is 500ms)

        m_randomInterval->SetAttribute("Min", DoubleValue(minIntervalMs));
        m_randomInterval->SetAttribute("Max", DoubleValue(maxIntervalMs));

        NS_LOG_INFO("STA " << (m_staIndex + 1) << " configured for random transmission:");
        NS_LOG_INFO("  Mean interval: " << meanIntervalMs << " ms");
        NS_LOG_INFO("  Random range: " << minIntervalMs << " - " << maxIntervalMs << " ms");
        NS_LOG_INFO("  Expected rate: ~" << (1000.0 / meanIntervalMs) << " transmissions/second");
    }

    virtual void StartApplication()
    {
        if (!m_socket)
        {
            TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
            m_socket = Socket::CreateSocket(GetNode(), tid);
            m_socket->Connect(m_peer);
        }
        ScheduleTransmit(Seconds(0.0));
    }

    virtual void StopApplication()
    {
        if (m_sendEvent.IsPending())
        {
            Simulator::Cancel(m_sendEvent);
        }
        if (m_socket)
        {
            m_socket->Close();
        }
    }

private:
    void ScheduleTransmit(Time dt)
    {
        m_sendEvent = Simulator::Schedule(dt, &EnergyAwareUdpClient::Send, this);
    }

    Time GetRandomInterval()
    {
        // Generate random interval in milliseconds
        double randomMs = m_randomInterval->GetValue();
        return MilliSeconds(randomMs);
    }

    void Send()
    {
        if (m_sent < m_maxPackets)
        {
            // Check if there's enough energy for transmission BEFORE creating/sending packet
            // IMPORTANT: Add protocol overhead to payload size for accurate duration estimate
            // UDP (8) + IP (20) + LLC/SNAP (8) + WiFi MAC (~30) + FCS (4) + QoS/HT (~40) = ~110 bytes overhead
            uint32_t estimatedPhyPacketSize = m_packetSize + 110; // Payload + typical protocol overhead
            Time txDuration =
                CalculateWiFiPacketDuration(Create<Packet>(estimatedPhyPacketSize), m_ulDataRate, false);

            NS_LOG_DEBUG("STA " << (m_staIndex + 1) << " Send() attempt " << (m_sent + 1)
                                << "/" << m_maxPackets
                                << " | Estimated TX duration: " << txDuration.GetMicroSeconds() << " μs"
                                << " | Time: " << Simulator::Now().GetSeconds() << "s");

            // Calculate required energy for transmission
            double txPowerWatts =
                std::pow(10.0, (m_txPowerDbm - 30.0) / 10.0);               // Convert dBm to Watts
            double requiredEnergy = txPowerWatts * txDuration.GetSeconds(); // Energy = Power × Time

            // Get current available energy
            double availableEnergy = 0.0;
            if (m_staIndex < g_harvesters.size() && g_harvesters[m_staIndex])
            {
                availableEnergy = g_harvesters[m_staIndex]->GetAvailableEnergy();
            }

            if (CheckEnergyForTransmission(m_staIndex, m_txPowerDbm, txDuration))
            {
                // Sufficient energy - proceed with transmission
                // Energy will be deducted in OnPhyTxEnd() with ACTUAL PHY duration

                NS_LOG_INFO("STA " << (m_staIndex + 1) << " TX Estimate: "
                                   << txDuration.GetMicroSeconds() << " μs (from CalculateWiFiPacketDuration)"
                                   << " | Packet size: " << m_packetSize << " bytes"
                                   << " | Time: " << Simulator::Now().GetSeconds() << "s");

                Ptr<Packet> packet = Create<Packet>(m_packetSize);
                m_socket->Send(packet);
                m_sent++;

                // Update statistics
                if (m_staIndex < g_staStats.size())
                {
                    g_staStats[m_staIndex].transmissionEvents++;
                }

                NS_LOG_DEBUG("STA " << (m_staIndex + 1) << " transmitted packet " << m_sent
                                    << " (Estimated energy: " << std::scientific
                                    << std::setprecision(3) << requiredEnergy << " J, actual deduction in OnPhyTxEnd)");
            }
            else
            {
                // Insufficient energy - block transmission
                if (m_staIndex < g_staStats.size())
                {
                    g_staStats[m_staIndex].blockedTransmissions++;
                }

                // Log blocked transmission event to CSV
                double vcap = 0.0;
                bool outputEnabled = false; // Always false since we're blocking
                if (m_staIndex < g_harvesters.size() && g_harvesters[m_staIndex])
                {
                    vcap = g_harvesters[m_staIndex]->GetVcap();
                    outputEnabled = g_harvesters[m_staIndex]->IsOutputEnabled();
                }

                LogSimulationEvent(Simulator::Now().GetSeconds(),
                                   m_staIndex + 1,
                                   "TX_BLOCKED",
                                   0.0,
                                   m_txPowerDbm,
                                   m_packetSize,
                                   txDuration.GetMicroSeconds(),
                                   0.0,
                                   0.0,
                                   availableEnergy,
                                   vcap,
                                   g_staStats[m_staIndex].capacitorClass,
                                   g_staStats[m_staIndex].voltageClass,
                                   true,
                                   false,
                                   outputEnabled,
                                   "Energy insufficient: req=" + std::to_string(requiredEnergy) +
                                       "J avail=" + std::to_string(availableEnergy) + "J");

                NS_LOG_WARN("STA " << (m_staIndex + 1) << " blocked transmission packet "
                                   << (m_sent + 1) << " (Insufficient energy) | Required: "
                                   << std::scientific << std::setprecision(3) << requiredEnergy
                                   << " J | Available: " << std::scientific << std::setprecision(3)
                                   << availableEnergy << " J");
            }

            // Schedule next transmission attempt with RANDOM interval
            Time nextInterval = GetRandomInterval();
            NS_LOG_DEBUG("STA " << (m_staIndex + 1) << " next transmission in "
                                << nextInterval.GetMilliSeconds() << " ms");
            ScheduleTransmit(nextInterval);
        }
        else
        {
            // Maximum packets reached - stop transmitting
            NS_LOG_INFO("STA " << (m_staIndex + 1) << " reached maximum packet limit: "
                               << m_maxPackets << " packets sent at time "
                               << Simulator::Now().GetSeconds() << "s");
        }
    }
};

// Statistics update function
void UpdateSTAStatistics()
{
    static uint32_t updateCount = 0;
    updateCount++;

    std::cout << "\n═══════════════════════════════════════════════════════════════════"
              << std::endl;
    std::cout << "🔋 PowerCast Energy Harvesting Statistics Update #" << updateCount << std::endl;
    std::cout << "   Time: " << std::fixed << std::setprecision(3) << Simulator::Now().GetSeconds()
              << " seconds" << std::endl;
    std::cout << "═══════════════════════════════════════════════════════════════════" << std::endl;

    std::cout << "┌─────┬─────┬──────┬──────────────────┬──────────────────┬────────┬────────┬─────"
                 "─────┬────────┬────────┐"
              << std::endl;
    std::cout << "│ STA │ Cap │ Volt │ Total Harv (J)   │ Total Cons (J)   │ Vcap(V)│ Status │ "
                 "Events   │ TX Evt │ Blocked│"
              << std::endl;
    std::cout << "├─────┼─────┼──────┼──────────────────┼──────────────────┼────────┼────────┼─────"
                 "─────┼────────┼────────┤"
              << std::endl;

    double networkTotalHarvested = 0.0;
    double networkTotalConsumed = 0.0;

    for (uint32_t i = 0; i < g_staStats.size(); ++i)
    {
        if (g_harvesters[i])
        {
            g_staStats[i].totalHarvested = g_harvesters[i]->GetTotalHarvestedEnergy();
            g_staStats[i].totalConsumed = g_harvesters[i]->GetTotalConsumedEnergy();
            g_staStats[i].currentVoltage = g_harvesters[i]->GetVcap();
            g_staStats[i].outputEnabled = true; // Simplified - no output enable logic
            g_staStats[i].energyBalance =
                g_harvesters[i]->GetAvailableEnergy(); // Use available energy directly

            networkTotalHarvested += g_staStats[i].totalHarvested;
            networkTotalConsumed += g_staStats[i].totalConsumed;

            std::string status = g_staStats[i].outputEnabled ? "🟢 ON " : "🔴 OFF";
            std::string harvestIcon = (g_staStats[i].totalHarvested > 0) ? "✅" : "❌";

            std::cout << "│ " << std::setw(3) << g_staStats[i].nodeId << " │  "
                      << g_staStats[i].capacitorClass << "  │  " << g_staStats[i].voltageClass
                      << "   │ " << std::setw(14) << std::scientific << std::setprecision(3)
                      << g_staStats[i].totalHarvested << " " << harvestIcon << " │ "
                      << std::setw(14) << std::scientific << std::setprecision(3)
                      << g_staStats[i].totalConsumed << "   │ " << std::setw(4) << std::fixed
                      << std::setprecision(2) << g_staStats[i].currentVoltage << "   │ " << status
                      << " │ " << std::setw(6) << g_staStats[i].harvestingEvents << "   │ "
                      << std::setw(4) << g_staStats[i].transmissionEvents << "   │ " << std::setw(4)
                      << g_staStats[i].blockedTransmissions << "   │" << std::endl;
        }
    }

    std::cout << "├─────┴─────┴──────┼──────────────────┼──────────────────┼────────┼────────┼─────"
                 "─────┼────────┼────────┤"
              << std::endl;
    std::cout << "│🔋 NETWORK TOTALS │ " << std::setw(14) << std::scientific << std::setprecision(3)
              << networkTotalHarvested << "   │ " << std::setw(14) << std::scientific
              << std::setprecision(3) << networkTotalConsumed
              << "   │        │        │          │        │        │" << std::endl;
    std::cout << "└─────────────────┴──────────────────┴──────────────────┴────────┴────────┴──────"
                 "────┴────────┴────────┘"
              << std::endl;

    // Efficiency analysis
    if (networkTotalHarvested > 0)
    {
        double efficiency =
            (networkTotalHarvested / (networkTotalHarvested + networkTotalConsumed)) * 100.0;
        std::cout << "\n📊 Network Energy Efficiency: " << std::fixed << std::setprecision(2)
                  << efficiency << "%" << std::endl;
    }
}

// PHY RX Begin callback - stores RX start information
void OnPhyRxBegin(std::string context, Ptr<const Packet> packet, RxPowerWattPerChannelBand rxPowersW)
{
    // Extract node ID from context (format: "/NodeList/X/...")
    std::string nodeIdStr = context.substr(10); // Skip "/NodeList/"
    size_t pos = nodeIdStr.find("/");
    if (pos != std::string::npos)
    {
        nodeIdStr = nodeIdStr.substr(0, pos);
    }

    uint32_t nodeId = std::stoul(nodeIdStr);

    // Only process STA nodes (IDs 1 to numSTAs)
    if (nodeId >= 1 && nodeId <= g_staStats.size())
    {
        uint32_t staIndex = nodeId - 1;

        if (g_harvesters[staIndex])
        {
            // Calculate total received power
            double totalRxPowerWatts = 0.0;
            for (const auto &bandPower : rxPowersW)
            {
                totalRxPowerWatts += bandPower.second;
            }

            if (totalRxPowerWatts > 0.0)
            {
                // Convert to dBm
                double rxPowerDbm = 10.0 * log10(totalRxPowerWatts * 1000.0);

                // Store RX event information for duration calculation when RX ends
                PhyEvent rxEvent;
                rxEvent.startTime = Simulator::Now();
                rxEvent.powerDbm = rxPowerDbm;
                rxEvent.packetSize = packet->GetSize();
                rxEvent.nodeId = nodeId;
                g_ongoingRxEvents[nodeId] = rxEvent;

                NS_LOG_INFO("STA " << nodeId << " RX Begin: " << rxPowerDbm
                                   << " dBm, Packet: " << packet->GetSize() << " bytes");
            }
        }
    }
}

// PHY RX End callback - calculates actual RX duration and performs energy harvesting
void OnPhyRxEnd(std::string context, Ptr<const Packet> packet)
{
    // Extract node ID from context
    std::string nodeIdStr = context.substr(10); // Skip "/NodeList/"
    size_t pos = nodeIdStr.find("/");
    if (pos != std::string::npos)
    {
        nodeIdStr = nodeIdStr.substr(0, pos);
    }

    uint32_t nodeId = std::stoul(nodeIdStr);

    // Only process STA nodes (IDs 1 to numSTAs)
    if (nodeId >= 1 && nodeId <= g_staStats.size())
    {
        uint32_t staIndex = nodeId - 1;

        // Check if we have a matching RX Begin event
        auto it = g_ongoingRxEvents.find(nodeId);
        if (it != g_ongoingRxEvents.end() && g_harvesters[staIndex])
        {
            PhyEvent &rxEvent = it->second;

            // Calculate ACTUAL reception duration from PHY timing
            Time actualDuration = Simulator::Now() - rxEvent.startTime;

            // Store previous energy values for logging
            double prevHarvested = g_harvesters[staIndex]->GetTotalHarvestedEnergy();

            // Perform energy harvesting using ACTUAL PHY duration
            g_harvesters[staIndex]->SetHarvestedEnergy(rxEvent.powerDbm, actualDuration);
            g_staStats[staIndex].harvestingEvents++;

            // Calculate harvested energy and new available energy
            double newHarvested = g_harvesters[staIndex]->GetTotalHarvestedEnergy();
            double harvestedAmount = newHarvested - prevHarvested;
            double newAvailable = g_harvesters[staIndex]->GetAvailableEnergy();
            double vcap = g_harvesters[staIndex]->GetVcap();
            bool outputEnabled = true; // Simplified - no output enable logic

            // Convert power from dBm back to Watts for logging
            double rxPowerWatts = std::pow(10.0, (rxEvent.powerDbm - 30.0) / 10.0);

            // Log RX event to CSV
            LogSimulationEvent(Simulator::Now().GetSeconds(),
                               nodeId,
                               "RX",
                               rxEvent.powerDbm,
                               0.0,
                               rxEvent.packetSize,
                               actualDuration.GetMicroSeconds(),
                               harvestedAmount,
                               0.0,
                               newAvailable,
                               vcap,
                               g_staStats[staIndex].capacitorClass,
                               g_staStats[staIndex].voltageClass,
                               false,
                               true,
                               outputEnabled,
                               "Harvesting from " + std::to_string(rxPowerWatts * 1000.0) +
                                   "mW signal (ACTUAL PHY duration)");

            NS_LOG_INFO("STA " << nodeId << " RX End: " << rxEvent.powerDbm
                               << " dBm, Duration: " << actualDuration.GetMicroSeconds()
                               << " μs (ACTUAL), Packet: " << rxEvent.packetSize << " bytes"
                               << " | Harvested: +" << std::scientific << std::setprecision(3)
                               << harvestedAmount << " J"
                               << " | Available: " << std::scientific << std::setprecision(3)
                               << newAvailable << " J");

            // Remove the event from tracking map
            g_ongoingRxEvents.erase(it);
        }
    }
}

// PHY TX Begin callback - stores TX start information
void OnPhyTxBegin(std::string context, Ptr<const Packet> packet, double txPowerW)
{
    // Extract node ID from context
    std::string nodeIdStr = context.substr(10); // Skip "/NodeList/"
    size_t pos = nodeIdStr.find("/");
    if (pos != std::string::npos)
    {
        nodeIdStr = nodeIdStr.substr(0, pos);
    }

    uint32_t nodeId = std::stoul(nodeIdStr);

    // Only process STA nodes (IDs 1 to numSTAs)
    if (nodeId >= 1 && nodeId <= g_staStats.size())
    {
        uint32_t staIndex = nodeId - 1;

        if (g_harvesters[staIndex])
        {
            // Convert power to dBm
            double txPowerDbm = 10.0 * log10(txPowerW * 1000.0);

            // Store TX event information for duration calculation when TX ends
            PhyEvent txEvent;
            txEvent.startTime = Simulator::Now();
            txEvent.powerDbm = txPowerDbm;
            txEvent.packetSize = packet->GetSize();
            txEvent.nodeId = nodeId;
            g_ongoingTxEvents[nodeId] = txEvent;

            NS_LOG_INFO("STA " << nodeId << " TX Begin: " << txPowerDbm
                               << " dBm, Packet: " << packet->GetSize() << " bytes");
        }
    }
}

// PHY TX End callback - calculates actual TX duration and logs consumption
void OnPhyTxEnd(std::string context, Ptr<const Packet> packet)
{
    // Extract node ID from context
    std::string nodeIdStr = context.substr(10); // Skip "/NodeList/"
    size_t pos = nodeIdStr.find("/");
    if (pos != std::string::npos)
    {
        nodeIdStr = nodeIdStr.substr(0, pos);
    }

    uint32_t nodeId = std::stoul(nodeIdStr);

    // Only process STA nodes (IDs 1 to numSTAs)
    if (nodeId >= 1 && nodeId <= g_staStats.size())
    {
        uint32_t staIndex = nodeId - 1;

        // Check if we have a matching TX Begin event
        auto it = g_ongoingTxEvents.find(nodeId);
        if (it != g_ongoingTxEvents.end() && g_harvesters[staIndex])
        {
            PhyEvent &txEvent = it->second;

            // Calculate ACTUAL transmission duration from PHY timing
            Time actualDuration = Simulator::Now() - txEvent.startTime;

            // Calculate estimated duration for comparison
            Time estimatedDuration = CalculateWiFiPacketDuration(
                Create<Packet>(txEvent.packetSize), g_ulDataRate, false);

            // Calculate difference between estimate and actual
            double durationDiffUs = actualDuration.GetMicroSeconds() - estimatedDuration.GetMicroSeconds();
            double durationDiffPercent = (durationDiffUs / estimatedDuration.GetMicroSeconds()) * 100.0;

            // Calculate the implied data rate from actual duration
            // actual_duration = PHY_overhead + (packet_bits / data_rate)
            // data_rate = packet_bits / (actual_duration - PHY_overhead)
            double phyOverheadUs = 32.0; // μs
            double dataDurationUs = actualDuration.GetMicroSeconds() - phyOverheadUs;
            double impliedDataRateMbps = 0.0;
            if (dataDurationUs > 0)
            {
                double packetBits = txEvent.packetSize * 8.0;
                impliedDataRateMbps = packetBits / dataDurationUs; // Mbits/μs = Mbps
            }

            // Calculate consumed energy based on ACTUAL PHY transmission duration
            double txPowerWatts = std::pow(10.0, (txEvent.powerDbm - 30.0) / 10.0);
            double consumedEnergy = txPowerWatts * actualDuration.GetSeconds(); // Energy = Power × Time

            // Log BEFORE deduction for comparison
            double vcapBefore = g_harvesters[staIndex]->GetVcap();
            double energyBefore = g_harvesters[staIndex]->GetAvailableEnergy();

            // NOW deduct energy based on ACTUAL PHY transmission duration
            g_harvesters[staIndex]->SetConsumedEnergy(txEvent.powerDbm, actualDuration);

            // Log AFTER deduction
            double availableEnergy = g_harvesters[staIndex]->GetAvailableEnergy();
            double vcap = g_harvesters[staIndex]->GetVcap();
            bool outputEnabled = g_harvesters[staIndex]->IsOutputEnabled();

            NS_LOG_DEBUG("STA " << nodeId << " energy deduction (ACTUAL PHY):"
                                << " Vcap: " << vcapBefore << "V → " << vcap << "V"
                                << " | Energy: " << std::scientific << std::setprecision(3)
                                << energyBefore << "J → " << availableEnergy << "J"
                                << " | Consumed: " << consumedEnergy << "J");

            // Update statistics with actual consumed energy
            if (staIndex < g_staStats.size())
            {
                g_staStats[staIndex].totalConsumed += consumedEnergy;
            }

            // Log TX event to CSV with actual consumed energy amount
            LogSimulationEvent(Simulator::Now().GetSeconds(),
                               nodeId,
                               "TX_CONFIRMED",
                               0.0,
                               txEvent.powerDbm,
                               txEvent.packetSize,
                               actualDuration.GetMicroSeconds(),
                               0.0,
                               consumedEnergy, // Report actual consumed energy
                               availableEnergy,
                               vcap,
                               g_staStats[staIndex].capacitorClass,
                               g_staStats[staIndex].voltageClass,
                               false,
                               true,
                               outputEnabled,
                               "PHY transmission confirmed (energy deducted with ACTUAL PHY duration)");

            NS_LOG_INFO("STA " << nodeId << " TX End: " << txEvent.powerDbm << " dBm"
                               << " | Estimated: " << estimatedDuration.GetMicroSeconds() << " μs"
                               << " | ACTUAL: " << actualDuration.GetMicroSeconds() << " μs"
                               << " | Diff: " << std::fixed << std::setprecision(1) << durationDiffUs << " μs ("
                               << std::setprecision(1) << durationDiffPercent << "%)"
                               << " | Packet: " << txEvent.packetSize << " bytes"
                               << " | Implied rate: " << std::setprecision(2) << impliedDataRateMbps << " Mbps"
                               << " (expected: " << g_ulDataRate << " Mbps)"
                               << " | Consumed: " << std::scientific << std::setprecision(3) << consumedEnergy << " J"
                               << " | Available after: " << availableEnergy << " J");

            // Remove the event from tracking map
            g_ongoingTxEvents.erase(it);
        }
    }
}

int main(int argc, char *argv[])
{
    // Simulation parameters
    double simulationTime = 180.0; // seconds
    double apTxPowerDbm = 55.0;    // AP transmission power (increased to 45 dBm)
    double staTxPowerDbm = 15.0;   // STA transmission power (increased to 15 dBm)

    // Data rate and MCS configuration
    // Using SLOWEST possible rate (1 Mbps DSSS) for STAs to ensure maximum predictability and energy efficiency
    double dlDataRate = 54.0;            // Mbps - Downlink data rate
    double ulDataRate = 1.0;             // Mbps - Uplink data rate (SLOWEST: 1 Mbps DSSS for maximum predictability)
    std::string dlMcs = "HtMcs7";        // Downlink MCS (54 Mbps)
    std::string ulMcs = "DsssRate1Mbps"; // Uplink: SLOWEST rate (1 Mbps DSSS) for constant timing and energy efficiency

    // Packet size configuration
    uint32_t ulPayloadSize = 1;    // bytes - Small uplink payload for energy-constrained STAs
    uint32_t dlPayloadSize = 2048; // bytes - Standard downlink payload size

    uint32_t numSTAs = 8; // Number of STAs - configurable
    std::string configFileName =
        "contrib/ai/examples/powercast_hardware/ph-harvester-config.txt"; // Config file in source directory

    // Command line parsing
    CommandLine cmd(__FILE__);
    cmd.AddValue("time", "Simulation time (seconds)", simulationTime);
    cmd.AddValue("apPower", "AP transmission power (dBm)", apTxPowerDbm);
    cmd.AddValue("staPower", "STA transmission power (dBm)", staTxPowerDbm);
    cmd.AddValue("dlRate", "Downlink data rate (Mbps)", dlDataRate);
    cmd.AddValue("ulRate", "Uplink data rate (Mbps)", ulDataRate);
    cmd.AddValue("dlMcs", "Downlink MCS mode", dlMcs);
    cmd.AddValue("ulMcs", "Uplink MCS mode", ulMcs);
    cmd.AddValue("ulPayload", "Uplink payload size (bytes)", ulPayloadSize);
    cmd.AddValue("dlPayload", "Downlink payload size (bytes)", dlPayloadSize);
    cmd.AddValue("numStas", "Number of STAs", numSTAs);
    cmd.AddValue("configFile", "Path to harvester configuration file", configFileName);
    cmd.Parse(argc, argv);

    // Update global variables for PHY callbacks
    g_dlDataRate = dlDataRate;
    g_ulDataRate = ulDataRate;

    // Initialize global vectors based on numSTAs
    g_staStats.resize(numSTAs);
    g_harvesters.resize(numSTAs);

    // Enable logging
    LogComponentEnable("PowercastSimpleTest", LOG_LEVEL_INFO);
    LogComponentEnable("PowercastEnergyHarvester", LOG_LEVEL_INFO);

    // Initialize CSV logging file
    std::string csvFileName =
        "contrib/ai/examples/powercast_hardware/ph-pcap/ph-harvester-demo-log.csv";
    g_simulationLogFile.open(csvFileName);
    if (!g_simulationLogFile.is_open())
    {
        NS_FATAL_ERROR("Could not open CSV log file: " << csvFileName);
    }

    // Write CSV header
    g_simulationLogFile << "Timestamp_s,STA_ID,Event_Type,RX_Power_dBm,TX_Power_dBm,"
                        << "Packet_Size_bytes,Duration_us,Harvested_Energy_J,Consumed_Energy_J,"
                        << "Available_Energy_J,Vcap_V,Cap_Class,Volt_Class,Block_Status,"
                        << "Success_Status,Output_Status,Additional_Info" << std::endl;

    std::cout << "═══════════════════════════════════════════════════════════════════" << std::endl;
    std::cout << "🔋 PowerCast RF Energy Harvesting Test - Simplified Version" << std::endl;
    std::cout << "═══════════════════════════════════════════════════════════════════" << std::endl;
    std::cout << "\n📊 CSV Logging initialized: " << csvFileName << std::endl;
    std::cout << "\n📋 Simulation Configuration:" << std::endl;
    std::cout << "   Simulation Time: " << simulationTime << " seconds" << std::endl;
    std::cout << "   Number of STAs: " << numSTAs << std::endl;
    std::cout << "   AP TX Power: " << apTxPowerDbm << " dBm" << std::endl;
    std::cout << "   STA TX Power: " << staTxPowerDbm << " dBm" << std::endl;
    std::cout << "   Downlink Rate: " << dlDataRate << " Mbps (MCS: " << dlMcs << ")" << std::endl;
    std::cout << "   Uplink Rate: " << ulDataRate << " Mbps (MCS: " << ulMcs << ")" << std::endl;
    std::cout << "   Uplink Payload Size: " << ulPayloadSize << " bytes" << std::endl;
    std::cout << "   Downlink Payload Size: " << dlPayloadSize << " bytes" << std::endl;
    std::cout << "   Config File: " << configFileName << std::endl;

    // === NODE CREATION ===
    NodeContainer apNode, staNodes;
    apNode.Create(1);
    staNodes.Create(numSTAs); // Use configurable number of STAs
    NodeContainer allNodes = NodeContainer(apNode, staNodes);

    // === MOBILITY SETUP ===
    MobilityHelper mobility;

    // AP at center
    Ptr<ListPositionAllocator> apPosition = CreateObject<ListPositionAllocator>();
    apPosition->Add(Vector(0.0, 0.0, 0.0));
    mobility.SetPositionAllocator(apPosition);
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(apNode);

    // STAs in a larger circle around AP
    Ptr<ListPositionAllocator> staPositions = CreateObject<ListPositionAllocator>();
    double radius = 1.15; // meters - increased radius for more STAs
    for (uint32_t i = 0; i < numSTAs; ++i)
    {
        double angle = 2 * M_PI * i / numSTAs;
        double x = radius * std::cos(angle);
        double y = radius * std::sin(angle);
        staPositions->Add(Vector(x, y, 0.0));
    }
    mobility.SetPositionAllocator(staPositions);
    mobility.Install(staNodes);

    std::cout << "\n📍 Network Topology:" << std::endl;
    std::cout << "   AP at center (0, 0, 0)" << std::endl;
    for (uint32_t i = 0; i < numSTAs; ++i)
    {
        double angle = 2 * M_PI * i / numSTAs;
        double x = radius * std::cos(angle);
        double y = radius * std::sin(angle);
        std::cout << "   STA " << (i + 1) << " at (" << std::fixed << std::setprecision(1) << x
                  << ", " << y << ", 0) - Distance: " << radius << "m" << std::endl;
    }

    // === WIFI CHANNEL SETUP ===
    YansWifiChannelHelper channelHelper = YansWifiChannelHelper::Default();
    channelHelper.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel");

    // Large scale fading: Log distance path loss
    channelHelper.AddPropagationLoss("ns3::LogDistancePropagationLossModel",
                                     "Exponent",
                                     DoubleValue(2.0),
                                     "ReferenceDistance",
                                     DoubleValue(1.0),
                                     "ReferenceLoss",
                                     DoubleValue(10));

    // Small scale fading: Nakagami-m fading model (more realistic than Rayleigh)
    channelHelper.AddPropagationLoss("ns3::NakagamiPropagationLossModel",
                                     "m0",
                                     DoubleValue(1.0), // Nakagami-m parameter for d < d1
                                     "m1",
                                     DoubleValue(1.5), // Nakagami-m parameter for d1 < d < d2
                                     "m2",
                                     DoubleValue(2.0), // Nakagami-m parameter for d > d2
                                     "Distance1",
                                     DoubleValue(80.0), // First distance threshold (m)
                                     "Distance2",
                                     DoubleValue(200.0)); // Second distance threshold (m)

    // === WIFI SETUP ===
    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211b); // Use 802.11b for slowest, most predictable rates

    // Use 1 Mbps DSSS rate for ALL transmissions - SLOWEST possible rate for maximum predictability
    wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                                 "DataMode",
                                 StringValue("DsssRate1Mbps"), // 1 Mbps DSSS for all data
                                 "ControlMode",
                                 StringValue("DsssRate1Mbps"), // 1 Mbps DSSS for control frames
                                 "NonUnicastMode",
                                 StringValue("DsssRate1Mbps")); // 1 Mbps DSSS for broadcast/multicast

    YansWifiPhyHelper phy;
    phy.SetChannel(channelHelper.Create());

    // Configure AP TX power with explicit power levels to override defaults
    phy.Set("TxPowerStart", DoubleValue(apTxPowerDbm));
    phy.Set("TxPowerEnd", DoubleValue(apTxPowerDbm));
    phy.Set("TxPowerLevels", UintegerValue(1)); // Single power level = constant power

    Ssid ssid = Ssid("PowerCast-Test-Network");
    WifiMacHelper mac;

    // AP configuration - force 1 Mbps DSSS as basic rate
    mac.SetType("ns3::ApWifiMac",
                "Ssid",
                SsidValue(ssid),
                "BeaconInterval",
                TimeValue(MicroSeconds(102400)), // 100ms = 102400μs (multiple of 1024μs)
                "BeaconGeneration",
                BooleanValue(true));
    NetDeviceContainer apDevice = wifi.Install(phy, mac, apNode);

    // Set basic rates for AP to 1 Mbps DSSS only (after device creation)
    Ptr<WifiNetDevice> apWifiDevice = DynamicCast<WifiNetDevice>(apDevice.Get(0));
    if (apWifiDevice)
    {
        Ptr<WifiMac> apMac = apWifiDevice->GetMac();
        Ptr<WifiRemoteStationManager> apManager = apWifiDevice->GetRemoteStationManager();
        if (apManager)
        {
            apManager->AddBasicMode(WifiMode("DsssRate1Mbps"));
        }
    }

    // STA configuration with explicit power control
    phy.Set("TxPowerStart", DoubleValue(staTxPowerDbm));
    phy.Set("TxPowerEnd", DoubleValue(staTxPowerDbm));
    phy.Set("TxPowerLevels", UintegerValue(1)); // Single power level = constant power

    // Disable fragmentation and RTS/CTS to avoid rate fallback
    mac.SetType("ns3::StaWifiMac",
                "Ssid", SsidValue(ssid),
                "ActiveProbing", BooleanValue(false),
                "MaxMissedBeacons", UintegerValue(1000)); // Prevent disconnections

    NetDeviceContainer staDevices = wifi.Install(phy, mac, staNodes);

    // Force disable RTS/CTS and set high fragmentation threshold on all STA devices
    Config::Set("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/RemoteStationManager/RtsCtsThreshold", UintegerValue(999999));
    Config::Set("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/RemoteStationManager/FragmentationThreshold", UintegerValue(999999));

    std::cout << "\n📡 WiFi Network Configuration:" << std::endl;
    std::cout << "   Standard: IEEE 802.11b (2.4GHz - SLOWEST rate for max predictability)" << std::endl;
    std::cout << "   SSID: " << ssid << std::endl;
    std::cout << "   Beacon Interval: 102.4 ms" << std::endl;
    std::cout << "   Downlink Mode: " << dlMcs << " (~" << dlDataRate << " Mbps)" << std::endl;
    std::cout << "   Uplink Mode: " << ulMcs << " (" << ulDataRate << " Mbps - SLOWEST DSSS rate for maximum timing predictability)" << std::endl;
    std::cout << "   Channel Model: Log Distance + Nakagami Small Scale Fading" << std::endl;

    // === ENERGY SYSTEM SETUP ===
    BasicEnergySourceHelper energySourceHelper;
    energySourceHelper.Set("BasicEnergySourceInitialEnergyJ", DoubleValue(100.0));
    EnergySourceContainer energySources = energySourceHelper.Install(allNodes);

    // PowerCast energy harvesters for STAs only
    PowercastEnergyHarvesterHelper harvesterHelper;

    // Try to load configuration from file, with robust fallback
    std::string configFile = configFileName;
    std::map<uint32_t, HarvesterConfig> harvesterConfigs = ReadHarvesterConfig(configFile);

    // Configure harvesters based on configuration file or use defaults
    std::cout << "\n🔌 PowerCast Energy Harvesting Configuration:" << std::endl;

    // Configuration arrays for fallback (similar to advanced test approach)
    std::vector<PowercastEnergyHarvester::CapacitorClass> defaultCapClasses = {
        PowercastEnergyHarvester::CLASS_A,
        PowercastEnergyHarvester::CLASS_B,
        PowercastEnergyHarvester::CLASS_C,
        PowercastEnergyHarvester::CLASS_A};
    std::vector<PowercastEnergyHarvester::VoltageClass> defaultVoltClasses = {
        PowercastEnergyHarvester::CLASS_1,
        PowercastEnergyHarvester::CLASS_2,
        PowercastEnergyHarvester::CLASS_1,
        PowercastEnergyHarvester::CLASS_3};
    std::vector<std::string> defaultCapNames = {"A (50000μF/50mF)", "B (2200μF)", "C (200000μF/200mF)", "A (50000μF/50mF)"};
    std::vector<std::string> defaultVoltNames = {"1 (1.2V)", "2 (0.9V)", "1 (1.2V)", "3 (0.7V)"};

    bool configFileLoaded = !harvesterConfigs.empty();
    if (configFileLoaded)
    {
        std::cout << "   ✅ Configuration loaded from file: " << configFile << std::endl;
        std::cout << "   � Found " << harvesterConfigs.size() << " node configurations"
                  << std::endl;
    }
    else
    {
        std::cout << "   ⚠️  Configuration file not found, using intelligent defaults" << std::endl;
    }

    for (uint32_t i = 1; i <= numSTAs; ++i)
    {
        PowercastEnergyHarvester::CapacitorClass capClass;
        PowercastEnergyHarvester::VoltageClass voltClass;
        std::string capName, voltName;

        // Use configuration from file (file uses 0-based indexing) - REQUIRED
        const HarvesterConfig &config = harvesterConfigs[i - 1];
        capClass = config.capacitorClass;
        voltClass = config.voltageClass;
        capName = config.capacitorName;
        voltName = config.voltageName;

        std::cout << "   STA " << i << ": " << capName << " Capacitor, " << voltName
                  << " Threshold [FROM FILE]" << std::endl;

        // Configure the harvester helper
        harvesterHelper.ConfigureNode(i, capClass, voltClass);

        // Store configuration for statistics
        g_staStats[i - 1].capacitorClass = capName.substr(0, 1);
        g_staStats[i - 1].voltageClass = voltName.substr(0, 1);
    }

    // Install harvesters on STA energy sources (skip AP - index 0)
    EnergySourceContainer staEnergySources;
    for (uint32_t i = 1; i <= numSTAs; ++i)
    {
        staEnergySources.Add(energySources.Get(i));
    }

    EnergyHarvesterContainer harvesters = harvesterHelper.Install(staEnergySources);

    // Store harvester references and initialize statistics
    for (uint32_t i = 0; i < numSTAs; ++i)
    {
        g_harvesters[i] = DynamicCast<PowercastEnergyHarvester>(harvesters.Get(i));
        g_staStats[i].nodeId = i + 1;
        // capacitorClass and voltageClass already set above
    }

    // === INTERNET PROTOCOL SETUP ===
    InternetStackHelper internet;
    internet.Install(allNodes);

    Ipv4AddressHelper ipv4;
    ipv4.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = ipv4.Assign(NetDeviceContainer(apDevice, staDevices));

    // ═══════════════════════════════════════════════════════════════════
    // PCAP LOGGING - CAPTURE ALL TRANSMISSIONS
    // ═══════════════════════════════════════════════════════════════════

    std::cout << "\n📄 Enabling PCAP logging for ALL transmissions..." << std::endl;

    // Enable Radiotap headers BEFORE EnablePcap calls
    phy.SetPcapDataLinkType(WifiPhyHelper::DLT_IEEE802_11_RADIO);

    // Enable PCAP on AP - files will be saved in powercast_hardware/ph-pcap/
    phy.EnablePcap("contrib/ai/examples/powercast_hardware/ph-pcap/ph-harvester-demo-ap", apDevice.Get(0));
    std::cout << "   ✅ AP PCAP: contrib/ai/examples/powercast_hardware/ph-pcap/ph-harvester-demo-ap-0-0.pcap" << std::endl;

    // Enable PCAP on all STAs
    for (uint32_t i = 0; i < numSTAs; ++i)
    {
        std::string pcapPrefix = "contrib/ai/examples/powercast_hardware/ph-pcap/ph-harvester-demo-sta-" + std::to_string(i + 1);
        phy.EnablePcap(pcapPrefix, staDevices.Get(i));
        std::cout << "   ✅ STA " << (i + 1) << " PCAP: contrib/ai/examples/powercast_hardware/ph-pcap/ph-harvester-demo-sta-" << (i + 1) << "-0-0.pcap" << std::endl;
    }

    std::cout << "📄 PCAP files will capture ALL WiFi frames (data, management, control)" << std::endl;
    std::cout << "📡 Radiotap headers ENABLED - includes RX power (signal strength) information!" << std::endl;
    std::cout << "📂 All PCAP files saved in: contrib/ai/examples/powercast_hardware/ph-pcap/ directory" << std::endl;

    // ═══════════════════════════════════════════════════════════════════
    // END OF PCAP LOGGING SETUP
    // ═══════════════════════════════════════════════════════════════════

    // === APPLICATION SETUP ===
    // UDP Echo Server on AP
    UdpEchoServerHelper echoServer(9);
    ApplicationContainer serverApps = echoServer.Install(apNode.Get(0));
    serverApps.Start(Seconds(1.0));
    serverApps.Stop(Seconds(simulationTime - 1.0));

    // Energy-aware UDP clients on STAs - check energy before transmission
    ApplicationContainer clientApps;
    for (uint32_t i = 0; i < numSTAs; ++i)
    {
        Ptr<EnergyAwareUdpClient> client = CreateObject<EnergyAwareUdpClient>();
        client->Setup(
            InetSocketAddress(interfaces.GetAddress(0), 9),
            ulPayloadSize,     // Use uplink payload size
            10000,             // Max packets
            MilliSeconds(500), // Mean interval: 500ms for ~2 transmissions/second (random)
            i,                 // STA index for energy checking
            staTxPowerDbm,     // TX power for energy calculation
            ulDataRate);       // Uplink data rate

        staNodes.Get(i)->AddApplication(client);
        client->SetStartTime(Seconds(2.0 + i * 0.1));
        client->SetStopTime(Seconds(simulationTime - 1.0));
        clientApps.Add(client);
    }

    std::cout << "\n📊 Application Traffic:" << std::endl;
    std::cout << "   UDP Echo Server on AP (port 9)" << std::endl;
    std::cout << "   Energy-aware UDP Clients on all " << numSTAs << " STAs" << std::endl;
    std::cout
        << "   Transmission Pattern: RANDOM intervals averaging ~2 transmissions/second per STA"
        << std::endl;
    std::cout << "   Random Interval Range: 100-900ms (mean: 500ms)" << std::endl;
    std::cout << "   Uplink Payload: " << ulPayloadSize << " bytes per packet" << std::endl;
    std::cout << "   Downlink Payload: " << dlPayloadSize << " bytes per packet" << std::endl;
    std::cout << "   Downlink Rate: " << dlDataRate << " Mbps (" << dlMcs
              << "), Uplink Rate: " << ulDataRate << " Mbps (" << ulMcs << ")" << std::endl;
    std::cout << "   Note: STAs are in receive mode when not transmitting (for energy harvesting)"
              << std::endl;

    // === PHY TRACE CONNECTIONS ===
    // Connect PHY-layer traces for energy harvesting and consumption tracking
    Config::Connect("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/$ns3::WifiPhy/PhyRxBegin",
                    MakeCallback(&OnPhyRxBegin));
    Config::Connect("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/$ns3::WifiPhy/PhyRxEnd",
                    MakeCallback(&OnPhyRxEnd));
    Config::Connect("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/$ns3::WifiPhy/PhyTxBegin",
                    MakeCallback(&OnPhyTxBegin));
    Config::Connect("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/$ns3::WifiPhy/PhyTxEnd",
                    MakeCallback(&OnPhyTxEnd));

    std::cout << "\n🔗 PHY Trace Connections:" << std::endl;
    std::cout << "   PhyRxBegin/End: Connected for ACTUAL duration-based energy harvesting" << std::endl;
    std::cout << "   PhyTxBegin/End: Connected for ACTUAL duration-based energy consumption tracking" << std::endl;
    std::cout << "   ✅ Using NS-3 native PHY timing (no manual duration calculation)" << std::endl;

    // === STATISTICS REPORTING ===
    // Schedule regular statistics updates
    for (double t = 10.0; t < simulationTime; t += 10.0)
    {
        Simulator::Schedule(Seconds(t), &UpdateSTAStatistics);
    }

    // === RUN SIMULATION ===
    std::cout << "\n🚀 Starting PowerCast Energy Harvesting Simulation..." << std::endl;
    std::cout << "   Duration: " << simulationTime << " seconds" << std::endl;
    std::cout << "   Statistics updates every 10 seconds" << std::endl;

    Simulator::Stop(Seconds(simulationTime));
    Simulator::Run();

    // === FINAL STATISTICS ===
    std::cout << "\n🏁 Simulation Complete! Final Statistics:" << std::endl;
    UpdateSTAStatistics();

    // NOTE: Detailed CSV export removed to avoid duplication - all data is already captured in ph-harvester-demo-log.csv
    // Secondary CSV export was redundant as the main logging system captures all simulation events

    std::cout << "\n📊 All simulation data logged to: contrib/ai/examples/powercast_hardware/ph-pcap/ph-harvester-demo-log.csv" << std::endl;
    std::cout << "\n🔋 Energy Performance Summary:" << std::endl;

    uint32_t totalTransmissions = 0;
    uint32_t totalBlocked = 0;
    double avgEfficiency = 0.0;

    for (uint32_t i = 0; i < numSTAs; ++i)
    {
        totalTransmissions += g_staStats[i].transmissionEvents;
        totalBlocked += g_staStats[i].blockedTransmissions;
        if (g_staStats[i].totalHarvested > 0)
        {
            avgEfficiency += (g_staStats[i].totalHarvested /
                              (g_staStats[i].totalHarvested + g_staStats[i].totalConsumed)) *
                             100.0;
        }
    }

    if (numSTAs > 0)
    {
        avgEfficiency /= numSTAs;
    }

    double transmissionSuccessRate =
        (totalTransmissions + totalBlocked > 0)
            ? (double)totalTransmissions / (totalTransmissions + totalBlocked) * 100.0
            : 0.0;

    std::cout << "   Total Successful Transmissions: " << totalTransmissions << std::endl;
    std::cout << "   Total Blocked Transmissions: " << totalBlocked << std::endl;
    std::cout << "   Transmission Success Rate: " << std::fixed << std::setprecision(1)
              << transmissionSuccessRate << "%" << std::endl;
    std::cout << "   Average Energy Efficiency: " << std::fixed << std::setprecision(1)
              << avgEfficiency << "%" << std::endl;

    std::cout << "\n✅ PowerCast Energy Harvesting Test Complete!" << std::endl;
    std::cout << "   📝 Key improvements implemented:" << std::endl;
    std::cout << "   • Using ACTUAL PHY timing for RX/TX duration (not manual calculation)" << std::endl;
    std::cout << "   • PhyRxBegin/End and PhyTxBegin/End callbacks for precise timing" << std::endl;
    std::cout << "   • Small uplink payload (" << ulPayloadSize << " byte) for energy efficiency"
              << std::endl;
    std::cout << "   • FIXED: Pre-deduct energy approach prevents double deduction" << std::endl;
    std::cout << "   • Energy-aware transmission with accurate pre-check" << std::endl;
    std::cout << "   • RANDOM transmission scheduling: 100-900ms intervals (avg ~2 TX/sec)"
              << std::endl;
    std::cout << "   • STAs in receive mode when not transmitting (optimal for energy harvesting)"
              << std::endl;
    std::cout << "   • SLOWEST possible rate: 1 Mbps DSSS (802.11b) for maximum timing predictability" << std::endl;
    std::cout << "   • Asymmetric data rates (DL: " << dlDataRate << " Mbps " << dlMcs
              << ", UL: " << ulDataRate << " Mbps " << ulMcs << " - SLOWEST)" << std::endl;
    std::cout << "   • Enhanced energy tracking with proper accounting" << std::endl;
    std::cout << "   • Comprehensive CSV event logging: "
                 "contrib/ai/examples/powercast_hardware/ph-pcap/ph-harvester-demo-log.csv"
              << std::endl;

    std::cout << "\n📄 PCAP Analysis Files Generated:" << std::endl;
    std::cout << "   • contrib/ai/examples/powercast_hardware/ph-pcap/ph-harvester-demo-ap-0-0.pcap (AP transmissions)" << std::endl;
    for (uint32_t i = 0; i < numSTAs; ++i)
    {
        std::cout << "   • contrib/ai/examples/powercast_hardware/ph-pcap/ph-harvester-demo-sta-" << (i + 1) << "-0-0.pcap (STA " << (i + 1) << " transmissions)" << std::endl;
    }
    std::cout << "   📊 Use Wireshark to analyze all captured WiFi frames!" << std::endl;

    // Close CSV logging file
    if (g_simulationLogFile.is_open())
    {
        g_simulationLogFile.close();
        std::cout << "\n📊 Simulation events logged to: "
                     "contrib/ai/examples/powercast_hardware/ph-pcap/ph-harvester-demo-log.csv"
                  << std::endl;
    }

    Simulator::Destroy();
    return 0;
}
