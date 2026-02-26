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
 * @file ph-harvester-hardware.h
 * @brief Advanced PowerCast RF Energy Harvester Interface
 *
 * Header file for production-grade RF energy harvesting device based on P21XXCSR-EVB
 * Band 6 specifications. Features advanced energy management with nominal/sunk/available
 * energy separation, realistic 2.4GHz efficiency modeling, comprehensive safety mechanisms,
 * and flexible configuration system for large-scale wireless network deployments.
 *
 * Key Features:
 * - Advanced three-tier energy model (nominal/sunk/available energy tracking)
 * - Realistic P21XXCSR-EVB Band 6 efficiency curve with 2450 MHz optimization
 * - Input power clamping at +15 dBm maximum rating with sensitivity handling
 * - Deep discharge protection with configurable DEEP_RATIO (50% of Vmin)
 * - Overshoot protection with SHOOT_RATIO (150% of Vmax) voltage limiting
 * - Critical error detection for unrealistic energy consumption scenarios
 * - Pre-transmission energy validation with CanSustainTransmission() method
 * - Output enable checking with 80% Vmin safety threshold
 * - Three-class capacitor system: CLASS_A, CLASS_B, CLASS_C (values from config file)
 * - Three-voltage threshold system: 1.2V, 0.9V, 0.7V operation modes
 * - Fixed 90% boost and PA efficiency from P21XXCSR-EVB specifications
 * - Comprehensive NS-3 integration with trace sources and attributes
 */

#ifndef PH_POWERCAST_ENERGY_HARVESTER_H
#define PH_POWERCAST_ENERGY_HARVESTER_H

#include "ns3/energy-harvester.h" // Base class for energy harvesting devices
#include "ns3/nstime.h"           // Time handling for simulation events
#include "ns3/traced-value.h"     // Monitoring and tracing system integration

namespace ns3
{
namespace energy
{

/**
 * \brief Advanced P21XXCSR Energy Harvester for 2.4GHz Wi-Fi Band
 *
 * Production-grade RF energy harvesting device based on P21XXCSR-EVB Band 6 specifications
 * with advanced energy management, realistic efficiency modeling, and comprehensive safety
 * mechanisms for accurate wireless network simulation and large-scale deployment scenarios.
 *
 * Hardware Model: P21XXCSR-EVB Band 6 (2.4GHz Wi-Fi)
 * - Operating Frequency: 2.4 GHz (2400-2500 MHz, center: 2450 MHz)
 * - Input Power Range: -40 to +15 dBm (realistic efficiency curve, clamped at +15 dBm)
 * - Boost Efficiency: 90% (from P21XXCSR-EVB datasheet specifications)
 * - Power Amplifier Efficiency: 90% (fixed for realistic transmission modeling)
 * - Energy Model: Advanced three-tier system (nominal/sunk/available energy)
 * - Safety Features: Deep discharge protection, overshoot protection, critical error detection
 * - Configuration: Three capacitor classes and three voltage threshold classes
 * - Validation: Pre-transmission energy checking and output enable status monitoring
 */
class PowercastEnergyHarvester : public EnergyHarvester
{
  public:
    /**
     * \brief Capacitor class enumeration based on P21XXCSR-EVB specifications
     *
     * Capacitor values are loaded from config file (no hardcoded defaults):
     * - CLASS_A: Small electrolytic (default: 500μF from config)
     * - CLASS_B: Medium electrolytic (default: 2200μF from config)
     * - CLASS_C: Large electrolytic (default: 20000μF from config)
     */
    enum CapacitorClass
    {
        CLASS_A,
        CLASS_B,
        CLASS_C
    };

    /**
     * \brief Voltage threshold class enumeration based on P21XXCSR-EVB specifications
     *
     * Based on P21XXCSR-EVB voltage threshold settings (JP3, JP4, JP5 jumper configurations):
     * - CLASS_1: 1.2V setting (VMAX=1.25V, VMIN=1.02V) - High voltage operation
     * - CLASS_2: 0.9V setting (VMAX=0.945V, VMIN=0.9V) - Medium voltage operation
     * - CLASS_3: 0.7V setting (VMAX=0.738V, VMIN=0.64V) - Low voltage operation
     */
    enum VoltageClass
    {
        CLASS_1, ///< 1.2V threshold setting (JP3) for high voltage operation
        CLASS_2, ///< 0.9V threshold setting (JP4) for medium voltage operation
        CLASS_3  ///< 0.7V threshold setting (JP5) for low voltage operation
    };

    /**
     * \brief Get TypeId for NS-3 object system integration
     * \return TypeId with P21XXCSR-EVB configuration attributes and trace sources
     */
    static TypeId GetTypeId();

    /**
     * \brief Set custom capacitor value for a class (in Farads)
     * \param capClass The capacitor class to configure
     * \param capacitanceF Capacitance value in Farads
     * 
     * Allows overriding default capacitor values from configuration files.
     * Must be called before creating harvester instances.
     */
    static void SetCapacitorValue(CapacitorClass capClass, double capacitanceF);

    /**
     * \brief Get capacitor value for a class (in Farads)
     * \param capClass The capacitor class to query
     * \return Capacitance value in Farads
     */
    static double GetCapacitorValue(CapacitorClass capClass);

    /**
     * \brief Default constructor with P21XXCSR-EVB specifications and advanced energy model
     *
     * Initializes harvester with default CLASS_A capacitor and CLASS_1 voltage (1.2V)
     * thresholds. Capacitor values must be loaded from config file before use.
     * Sets up advanced energy model with nominal/sunk/available energy separation
     * and applies realistic 2.4GHz efficiency curve with comprehensive safety mechanisms.
     */
    PowercastEnergyHarvester();

    /**
     * \brief Virtual destructor for proper inheritance cleanup
     */
    virtual ~PowercastEnergyHarvester() = default;

    // === CORE ENERGY MANAGEMENT METHODS ===

    /**
     * \brief Advanced RF Energy Harvesting with P21XXCSR-EVB Band 6 Modeling
     *
     * Converts received RF power at 2.4GHz to stored electrical energy using realistic
     * efficiency curve based on P21XXCSR-EVB Band 6 datasheet characterization.
     * Features input power clamping, overshoot protection, and advanced energy model
     * with nominal/sunk/available energy separation for accurate simulation.
     *
     * \param rxPowerDbm Received RF power in dBm (processed through realistic efficiency curve)
     * \param duration Signal reception duration for energy integration
     *
     * Efficiency Curve (P21XXCSR-EVB Band 6):
     * - Below -40 dBm: 0% (below sensitivity threshold)
     * - -40 to -30 dBm: 0% to 10% (sensitivity threshold region)
     * - -30 to -10 dBm: 10% to 40% (active harvesting region for typical Wi-Fi signals)
     * - -10 to 0 dBm: 40% to 70% (high power region for close proximity scenarios)
     * - 0 to +10 dBm: 70% to 85% (peak efficiency approach region)
     * - +10 to +15 dBm: 85% (maximum efficiency saturation region)
     * - Above +15 dBm: Clamped at 85% efficiency, +15 dBm power (hardware protection)
     *
     * Safety Features:
     * - Input power clamping at +15 dBm to prevent hardware damage
     * - Overshoot protection: voltage limited to 150% of Vmax
     * - Energy state consistency: automatic nominal/sunk/available energy recalculation
     */
    void SetHarvestedEnergy(double rxPowerDbm, Time duration);

    /**
     * \brief Advanced Energy Consumption with Safety Validation and PA Modeling
     *
     * Models energy drain during RF transmission with realistic 90% power amplifier
     * efficiency and comprehensive safety checks to prevent unrealistic energy consumption
     * that could lead to negative energy states or simulation inconsistencies.
     *
     * \param txPowerDbm Transmission power in dBm (converted to DC energy requirements)
     * \param duration Transmission duration for energy integration
     *
     * Safety Features:
     * - Critical error detection: prevents energy consumption exceeding available energy
     * - NS_FATAL_ERROR: forces simulation termination for debugging impossible energy states
     * - 5% numerical tolerance: accounts for floating-point precision limitations
     * - Comprehensive error logging: detailed information for debugging energy logic
     * - Automatic energy state updates: maintains nominal/sunk/available energy consistency
     *
     * Energy Model:
     * - RF Energy: Actual radiated power (E = P × t)
     * - DC Energy: Total energy consumption including PA inefficiency (E_DC = E_RF / η_PA)
     * - PA Efficiency: Fixed 90% based on P21XXCSR-EVB specifications
     */
    void SetConsumedEnergy(double txPowerDbm, Time duration);

    // === CONFIGURATION METHODS ===

    /**
     * \brief Set capacitor class with automatic energy state recalculation
     * \param capClass Capacitor class (CLASS_A, CLASS_B, CLASS_C - values from config file)
     */
    void SetCapacitorClass(CapacitorClass capClass);

    /**
     * \brief Set voltage threshold class with automatic energy state recalculation
     * \param voltClass Voltage class (CLASS_1: 1.2V, CLASS_2: 0.9V, CLASS_3: 0.7V)
     */
    void SetVoltageClass(VoltageClass voltClass);

    /**
     * \brief Complete harvester configuration with both capacitor and voltage classes
     * \param capClass Capacitor class for energy storage specification
     * \param voltClass Voltage threshold class for operational voltage range
     */
    void Configure(CapacitorClass capClass, VoltageClass voltClass);

    // === GETTER METHODS ===

    /**
     * \brief Get current available energy for transmission decisions
     * \return Current usable energy in Joules (excludes sunk energy)
     */
    double GetAvailableEnergy() const;

    /**
     * \brief Get current capacitor voltage
     * \return Current capacitor voltage in Volts
     */
    double GetVcap() const
    {
        return m_vcap;
    }

    /**
     * \brief Get P21XXCSR-EVB Band 6 harvesting efficiency for given input power
     * \param rxPowerDbm Input power level for efficiency calculation
     * \return Realistic 2.4GHz efficiency (0.0-0.85) at specified power level
     */
    double GetCurrentEfficiency(double rxPowerDbm) const;

    /**
     * \brief Pre-transmission energy sustainability validation
     *
     * Validates whether the harvester has sufficient available energy to sustain
     * a transmission without entering critical energy states. Should be called
     * before every transmission attempt to prevent unrealistic energy consumption.
     *
     * \param txPowerDbm Transmission power in dBm
     * \param duration Transmission duration
     * \return true if transmission can be sustained with current available energy
     *
     * Validation Process:
     * - Calculates total DC energy required (including PA efficiency)
     * - Compares against current available energy
     * - Returns false if insufficient energy available
     * - Provides comprehensive debug logging for energy decisions
     */
    bool CanSustainTransmission(double txPowerDbm, Time duration) const;

    /**
     * \brief Check harvester output enable status with safety margins
     *
     * Determines if the harvester can currently provide power based on dual
     * safety criteria: available energy and voltage safety threshold.
     *
     * \return true if output is enabled (energy available AND voltage above 80% Vmin)
     *
     * Safety Criteria:
     * - Available Energy: Must be greater than 0 Joules
     * - Voltage Safety: Must be above 80% of Vmin (deep discharge protection)
     * - Deep Discharge Protection: Prevents capacitor damage from over-discharge
     */
    bool IsOutputEnabled() const;

    /**
     * \brief Get transmission enable status with hysteresis control
     *
     * Returns the current transmission enable state controlled by hysteresis logic.
     * Transmission is enabled when capacitor reaches Vmax (full charge) and disabled
     * when capacitor drops to Vmin (empty charge). This prevents frequent on/off
     * cycling and ensures reliable operation.
     *
     * \return true if transmission is enabled by hysteresis control, false otherwise
     *
     * Hysteresis Behavior:
     * - Enable: Vcap >= Vmax (capacitor fully charged)
     * - Disable: Vcap <= Vmin (capacitor depleted)
     * - State maintained between thresholds (prevents oscillation)
     */
    bool IsTransmissionEnabled() const
    {
        return m_txEnabled;
    }

    /**
     * \brief Get current capacitor class
     * \return Current capacitor class
     */
    CapacitorClass GetCapacitorClass() const
    {
        return m_capacitorClass;
    }

    /**
     * \brief Get current voltage class
     * \return Current voltage threshold class
     */
    VoltageClass GetVoltageClass() const
    {
        return m_voltageClass;
    }

    // === ENERGY ACCOUNTING AND STATISTICS ===

    /**
     * \brief Get total cumulative harvested energy
     * \return Total energy harvested since initialization (Joules)
     */
    double GetTotalHarvestedEnergy() const
    {
        return m_totalHarvestedEnergy;
    }

    /**
     * \brief Get total cumulative consumed energy
     * \return Total energy consumed since initialization (Joules)
     */
    double GetTotalConsumedEnergy() const
    {
        return m_totalConsumedEnergy;
    }

    /**
     * \brief Get number of power cycling events
     * \return Count of on/off cycles (maintained for compatibility)
     */
    uint32_t GetOnOffCycles() const
    {
        return m_onOffCycles;
    }

  protected:
    /**
     * \brief NS-3 lifecycle cleanup
     */
    virtual void DoDispose() override;

  private:
    /**
     * \brief P21XXCSR-EVB Band 6 realistic efficiency calculation with input validation
     *
     * Implements realistic efficiency curve based on P21XXCSR-EVB Band 6 datasheet
     * characterization. Provides accurate efficiency modeling for 2450 MHz center
     * frequency operation with proper sensitivity threshold and saturation behavior.
     *
     * \param rxPowerDbm Input power level in dBm (should be pre-clamped to +15 dBm max)
     * \return Realistic efficiency value (0.0-0.85) based on hardware characteristics
     *
     * Efficiency Regions:
     * - Below -40 dBm: 0% (below minimum sensitivity)
     * - -40 to -30 dBm: 0% to 10% (sensitivity threshold region)
     * - -30 to -10 dBm: 10% to 40% (active region, 1.5%/dB slope)
     * - -10 to 0 dBm: 40% to 70% (high power region, 3%/dB slope)
     * - 0 to +10 dBm: 70% to 85% (peak efficiency approach, 1.5%/dB slope)
     * - +10 to +15 dBm: 85% (saturation plateau)
     * - Above +15 dBm: 85% with warning (hardware protection)
     */
    double CalculateRealistic24GHzEfficiency(double rxPowerDbm) const;

    /**
     * \brief Apply P21XXCSR-EVB capacitor class specifications
     * \param capClass Capacitor class to configure
     */
    void ApplyCapacitorClass(CapacitorClass capClass);

    /**
     * \brief Apply P21XXCSR-EVB voltage threshold class specifications
     * \param voltClass Voltage class to configure
     */
    void ApplyVoltageClass(VoltageClass voltClass);

    // === NS-3 ATTRIBUTE SYSTEM ACCESSORS ===
    void SetCapacitorClassAttribute(uint32_t capClass);
    uint32_t GetCapacitorClassAttribute() const;
    void SetVoltageClassAttribute(uint32_t voltClass);
    uint32_t GetVoltageClassAttribute() const;

    // === DEVICE PARAMETERS ===
    double m_vcap;     ///< Current capacitor voltage (V)
    double m_cStorage; ///< Storage capacitance (F)
    double m_vmax;     ///< Maximum operating voltage threshold (V)
    double m_vmin;     ///< Minimum operating voltage threshold (V)

    // === FIXED P21XXCSR-EVB PARAMETERS ===
    static const double BOOST_EFFICIENCY;    ///< Fixed 90% boost converter efficiency
    static const double PA_EFFICIENCY;       ///< Fixed 90% power amplifier efficiency
    static const double OPERATING_FREQUENCY; ///< Fixed 2450 MHz center frequency
    static const double DEEP_RATIO;          ///< Deep discharge protection ratio (0.5)
    static const double SHOOT_RATIO;         ///< Overshoot protection multiplier (1.5)

    // === DEVICE CONFIGURATION ===
    CapacitorClass m_capacitorClass; ///< Current capacitor class configuration
    VoltageClass m_voltageClass;     ///< Current voltage threshold class configuration

    // === ADVANCED ENERGY MODEL ===
    double m_availableEnergy;      ///< Usable energy for operations (J)
    double m_nominalEnergy;        ///< Total stored energy including sunk energy (J)
    double m_sunkEnergy;           ///< Unusable energy below deep discharge threshold (J)
    double m_totalHarvestedEnergy; ///< Cumulative harvested energy for statistics (J)
    double m_totalConsumedEnergy;  ///< Cumulative consumed energy for statistics (J)
    uint32_t m_onOffCycles;        ///< Power cycling events counter (for compatibility)

    // === HYSTERESIS STATE MANAGEMENT ===
    bool m_txEnabled;              ///< Transmission enable state with hysteresis control

    // === NS-3 TRACE SOURCES ===
    TracedValue<double> m_vcapTrace;            ///< Capacitor voltage trace for monitoring
    TracedValue<double> m_availableEnergyTrace; ///< Available energy trace for monitoring
    TracedValue<double> m_efficiencyTrace;      ///< Instantaneous efficiency trace for analysis

    // === STATIC CAPACITOR VALUE STORAGE (MUST be loaded from config files) ===
    static double s_capA_F;  ///< CLASS_A capacitance in Farads (MUST be set from config, no default)
    static double s_capB_F;  ///< CLASS_B capacitance in Farads (MUST be set from config, no default)
    static double s_capC_F;  ///< CLASS_C capacitance in Farads (MUST be set from config, no default)
};

} // namespace energy
} // namespace ns3

#endif // PH_POWERCAST_ENERGY_HARVESTER_H