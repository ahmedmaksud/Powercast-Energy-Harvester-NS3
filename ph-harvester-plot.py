#!/usr/bin/env python3
"""
PowerCast Energy Harvesting Visualization Script

Authors: Ahmed Maksud <amaks002@ucr.edu>
         SHINE Lab, Texas State University
         PI: Marcelo Menezes De Carvalho



Usage:
  python3 ph-harvester-plot.py --csv-file ph-pcap/ph-harvester-demo-log.csv
  python3 ph-harvester-plot.py --csv-file ph-pcap/ph-harvester-demo-log.csv --sta-id 1

Date: 2025
"""

import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.patches as patches
import os
import argparse

def get_capacitor_values_from_config():
    """Read capacitor values from config file.
    Returns dict mapping class letter (A, B, C) to value in μF."""
    config_file = 'ph-harvester-config.txt'
    cap_values = {}
    
    if not os.path.exists(config_file):
        print(f"⚠️ Config file '{config_file}' not found. Using default capacitor descriptions.")
        return None
    
    try:
        import re
        with open(config_file, 'r') as f:
            for line in f:
                line = line.strip()
                # Look for lines like: "# CAP_A 500" or "# CAP_B 2200"
                match = re.match(r'#\s*CAP_([ABC])\s+(\d+)', line)
                if match:
                    cap_class = match.group(1)
                    cap_value = int(match.group(2))
                    cap_values[cap_class] = cap_value
                    print(f"📋 Loaded CAP_{cap_class} = {cap_value} μF from config")
        
        if cap_values:
            return cap_values
        else:
            print(f"⚠️ No CAP_A/B/C values found in config file.")
            return None
    except Exception as e:
        print(f"⚠️ Error reading capacitor values from config: {e}")
        return None

def get_capacitor_description(cap_class, cap_values_from_config):
    """Get human-readable capacitor description based on class and config values."""
    if cap_values_from_config and str(cap_class).upper() in cap_values_from_config:
        value_uf = cap_values_from_config[str(cap_class).upper()]
        # Format based on size
        if value_uf >= 1000:
            return f"{value_uf}μF ({value_uf/1000:.1f}mF) Class {cap_class}"
        else:
            return f"{value_uf}μF Class {cap_class}"
    else:
        # Fallback to generic description
        return f'Class {cap_class} Capacitor'

def get_voltage_thresholds_from_config(voltage_class):
    """Get voltage thresholds based on voltage class from config file."""
    config_file = 'ph-harvester-config.txt'
    
    # Convert voltage_class to int for consistent handling
    try:
        voltage_class_int = int(voltage_class)
    except (ValueError, TypeError):
        raise ValueError(f"Invalid voltage class '{voltage_class}' - must be an integer")
    
    print(f"🔍 Parsing voltage class {voltage_class_int} from config file")
    
    if not os.path.exists(config_file):
        raise FileNotFoundError(f"Config file '{config_file}' not found. Cannot proceed without voltage thresholds.")
    
    try:
        with open(config_file, 'r') as f:
            content = f.read()
        
        # Parse voltage class definitions from comments
        lines = content.split('\n')
        for line in lines:
            line = line.strip()
            # Look for lines like: "#   1 = 1.2V thresholds (VMAX=1.25V, VMIN=1.02V)"
            if f'#   {voltage_class_int} =' in line and 'VMAX=' in line and 'VMIN=' in line:
                print(f"🔍 Found config line: {line}")
                
                # Extract VMAX, VMIN, and VCLASS values
                import re
                vmax_match = re.search(r'VMAX=([0-9.]+)V', line)
                vmin_match = re.search(r'VMIN=([0-9.]+)V', line)
                # Look for VCLASS= or extract from the class description (e.g., "1.2V thresholds")
                vclass_match = re.search(r'VCLASS=([0-9.]+)V', line)
                if not vclass_match:
                    # Try to extract from description like "1.2V thresholds"
                    desc_match = re.search(r'([0-9.]+)V thresholds', line)
                    if desc_match:
                        vclass_match = desc_match
                
                if vmax_match and vmin_match and vclass_match:
                    vmax = float(vmax_match.group(1))
                    vmin = float(vmin_match.group(1))
                    vclass = float(vclass_match.group(1))
                    
                    print(f"📋 Parsed from config - Voltage Class {voltage_class_int}: Vmax={vmax}V, Vclass={vclass:.3f}V, Vmin={vmin}V")
                    return vmax, vclass, vmin
                else:
                    missing = []
                    if not vmax_match: missing.append("VMAX")
                    if not vmin_match: missing.append("VMIN") 
                    if not vclass_match: missing.append("VCLASS/threshold")
                    raise ValueError(f"Could not parse {', '.join(missing)} from config line: {line}")
        
        # If voltage class not found in config
        raise ValueError(f"Voltage class {voltage_class_int} not found in config file '{config_file}'. "
                        f"Please check the config file contains a line like: '#   {voltage_class_int} = X.XV thresholds (VMAX=X.XXV, VMIN=X.XXV)'")
        
    except Exception as e:
        if isinstance(e, (ValueError, FileNotFoundError)):
            raise  # Re-raise our custom errors
        else:
            raise RuntimeError(f"Error reading config file '{config_file}': {e}")
def create_stratified_backgrounds(ax, events_df):
    """Create background colors for RX (blue), TX_CONFIRMED (green), and TX_BLOCKED (red) events using actual durations from CSV."""
    
    # Get plot limits
    xlim = ax.get_xlim()
    ylim = ax.get_ylim()
    
    # Calculate minimum visible width (0.1% of total time range)
    time_range = xlim[1] - xlim[0]
    min_visible_width = time_range * 0.001  # 0.1% of plot width
    
    # Track legend entries and background count
    legends_added = {'TX_CONFIRMED': False, 'TX_BLOCKED': False, 'RX': False}
    bg_count = {'TX_CONFIRMED': 0, 'TX_BLOCKED': 0, 'RX': 0}
    
    print(f"📊 Processing {len(events_df)} events for background coloring...")
    print(f"📊 Plot limits: X[{xlim[0]:.3f}, {xlim[1]:.3f}], Y[{ylim[0]:.3f}, {ylim[1]:.3f}]")
    print(f"📊 Minimum visible width: {min_visible_width:.6f}s ({min_visible_width*1e6:.1f}μs)")
    
    # Process events and add backgrounds across full plot height
    for _, event in events_df.iterrows():
        start_time = event['Timestamp_s']
        duration_us = event['Duration_us']
        duration_s = duration_us / 1e6  # Convert microseconds to seconds
        event_type = event['Event_Type']
        
        # Skip events with invalid durations
        if pd.isna(duration_us) or duration_us <= 0:
            continue
            
        # Skip events outside plot time range
        if start_time < xlim[0] or start_time > xlim[1]:
            continue
        
        # Use minimum visible width for very small durations
        display_width = max(duration_s, min_visible_width)
            
        # TX_CONFIRMED events (including TX) - Green background
        if event_type in ['TX_CONFIRMED'] and duration_s > 0:
            rect = patches.Rectangle(
                (start_time, ylim[0]), display_width, ylim[1] - ylim[0],
                facecolor='lightgreen', alpha=0.6, linewidth=0,
                label='TX Confirmed' if not legends_added['TX_CONFIRMED'] else ""
            )
            ax.add_patch(rect)
            bg_count['TX_CONFIRMED'] += 1
            if not legends_added['TX_CONFIRMED']:
                print(f"  🟢 Adding TX_CONFIRMED background: {start_time:.3f}s duration {duration_us}μs display_width={display_width:.6f}s")
                legends_added['TX_CONFIRMED'] = True
        
        # TX_BLOCKED events - Red background
        elif event_type == 'TX_BLOCKED' and duration_s > 0:
            rect = patches.Rectangle(
                (start_time, ylim[0]), display_width, ylim[1] - ylim[0],
                facecolor='lightcoral', alpha=0.6, linewidth=0,
                label='TX Blocked' if not legends_added['TX_BLOCKED'] else ""
            )
            ax.add_patch(rect)
            bg_count['TX_BLOCKED'] += 1
            if not legends_added['TX_BLOCKED']:
                print(f"  🔴 Adding TX_BLOCKED background: {start_time:.3f}s duration {duration_us}μs display_width={display_width:.6f}s")
                legends_added['TX_BLOCKED'] = True
            
        # RX events - Blue background
        elif event_type == 'RX' and duration_s > 0:
            rect = patches.Rectangle(
                (start_time, ylim[0]), display_width, ylim[1] - ylim[0],
                facecolor='lightblue', alpha=0.6, linewidth=0,
                label='RX Period' if not legends_added['RX'] else ""
            )
            ax.add_patch(rect)
            bg_count['RX'] += 1
            if not legends_added['RX']:
                print(f"  � Adding RX background: {start_time:.3f}s duration {duration_us}μs display_width={display_width:.6f}s")
                legends_added['RX'] = True
    
    print(f"📊 Background summary: TX_CONFIRMED={bg_count['TX_CONFIRMED']} rectangles, TX_BLOCKED={bg_count['TX_BLOCKED']} rectangles, RX={bg_count['RX']} rectangles")
    return legends_added

def create_single_plot(sta_data, sta_id, voltage_class, cap_values_from_config=None):
    """Create single plot showing only capacitor voltage with thresholds."""
    
    # Get STA configuration info
    cap_class = sta_data['Cap_Class'].iloc[0] if not sta_data.empty else 'A'
    volt_class = sta_data['Volt_Class'].iloc[0] if not sta_data.empty else '1'
    
    # Get capacitor description from config file
    cap_description = get_capacitor_description(cap_class, cap_values_from_config)
    
    # Map voltage class to actual values
    volt_info = {
        '1': '1.2V Class',
        '2': '0.9V Class',
        '3': '0.7V Class'
    }
    
    volt_description = volt_info.get(str(volt_class), f'Class {volt_class}')
    
    # Create figure and single axis with more space for external info box
    fig, ax1 = plt.subplots(figsize=(16, 8))
    
    # Enhanced title with STA configuration info
    fig.suptitle(f'PowerCast Energy Harvesting - STA {sta_id}\n' +
                f'Capacitor: {cap_description} | Voltage: {volt_description}', 
                fontsize=16, fontweight='bold')
    
    # Single axis: Capacitor Voltage
    ax1.set_xlabel('Time (seconds)', fontsize=12, fontweight='bold')
    ax1.set_ylabel('Capacitor Voltage (V)', fontsize=12, fontweight='bold')
    ax1.plot(sta_data['Timestamp_s'], sta_data['Vcap_V'], 
             color='tab:green', linewidth=2, label='Vcap', alpha=0.8)
    ax1.grid(True, alpha=0.3)
    
    # Add voltage threshold lines based on actual data
    try:
        vmax, vthreshold, vmin = get_voltage_thresholds_from_config(voltage_class)
    except (ValueError, FileNotFoundError, RuntimeError) as e:
        print(f"❌ Error getting voltage thresholds: {e}")
        print("❌ Cannot proceed without valid voltage thresholds. Please check your config file.")
        return None
    
    # Deep discharge limit (50% of Vmin based on PowerCast implementation)
    deep_discharge_limit = vmin * 0.5
    
    # Regulatory thresholds (for device operation)
    ax1.axhline(y=vthreshold, color='orange', linestyle='--', 
                alpha=0.8, linewidth=2, label=f'Vclass ({vthreshold:.3f}V)')
    ax1.axhline(y=vmax, color='red', linestyle='--', 
                alpha=0.8, linewidth=2, label=f'Vmax ({vmax}V)')
    ax1.axhline(y=vmin, color='purple', linestyle='--', 
                alpha=0.8, linewidth=2, label=f'Vmin ({vmin}V)')
    ax1.axhline(y=deep_discharge_limit, color='darkred', linestyle='-', 
                alpha=0.8, linewidth=2, label=f'Deep Discharge Limit ({deep_discharge_limit:.3f}V)')
    
    # Physical safety limit (20% above Vmax)
    safety_limit = vmax * 1.2
    ax1.axhline(y=safety_limit, color='darkred', linestyle=':', 
                alpha=0.6, linewidth=1.5, label=f'Safety Limit ({safety_limit:.3f}V)')
    
    # Set voltage axis limits to show safety margin
    voltage_range = max(sta_data['Vcap_V'].max(), safety_limit) - min(sta_data['Vcap_V'].min(), deep_discharge_limit)
    margin = voltage_range * 0.1
    ax1.set_ylim(min(sta_data['Vcap_V'].min(), deep_discharge_limit) - margin, 
                 max(sta_data['Vcap_V'].max(), safety_limit) + margin)
    
    # Add stratified backgrounds (use ax1 for consistent coordinate system)
    legend_status = create_stratified_backgrounds(ax1, sta_data)
    
    # Calculate and display STA statistics
    rx_events = len(sta_data[sta_data['Event_Type'] == 'RX'])
    tx_events = len(sta_data[sta_data['Event_Type'] == 'TX_CONFIRMED'])
    blocked_events = len(sta_data[sta_data['Event_Type'] == 'TX_BLOCKED'])
    
    # Calculate energy statistics
    total_harvested = sta_data['Harvested_Energy_J'].sum()
    total_consumed = sta_data['Consumed_Energy_J'].sum()
    final_energy = sta_data['Available_Energy_J'].iloc[-1] if not sta_data.empty else 0
    final_voltage = sta_data['Vcap_V'].iloc[-1] if not sta_data.empty else 0
    
    # Calculate transmission success rate
    total_attempts = tx_events + blocked_events
    success_rate = (tx_events / total_attempts * 100) if total_attempts > 0 else 0
    
    # Create statistics text box (positioned outside plot area)
    stats_text = f'''STA {sta_id} Statistics:
Capacitor: {cap_description}
Voltage: {volt_description}

Energy Performance:
• Total Harvested: {total_harvested:.2e} J
• Total Consumed: {total_consumed:.2e} J
• Final Available: {final_energy:.2e} J
• Final Vcap: {final_voltage:.3f} V

Transmission Performance:
• RX Events: {rx_events}
• Successful TX: {tx_events}
• Blocked TX: {blocked_events}
• Success Rate: {success_rate:.1f}%'''
    
    # Create legend first (positioned on the right side, upper area)
    lines1, labels1 = ax1.get_legend_handles_labels()
    fig.legend(lines1, labels1, loc='upper right', 
               bbox_to_anchor=(0.91, 0.88), fontsize=9)####
    
    # Position info box outside the plot area (right side, below legend)
    props = dict(boxstyle='round,pad=0.5', facecolor='lightblue', alpha=0.9, edgecolor='navy')
    fig.text(0.76, 0.52, stats_text, fontsize=9, verticalalignment='top',#### 
             bbox=props, family='monospace', transform=fig.transFigure)
    
    # Adjust layout to accommodate external info box and legend on the right
    plt.subplots_adjust(left=0.06, right=0.75, top=0.88, bottom=0.1)####
    
    return fig

def main():
    """Main function."""
    
    # Parse command line arguments
    parser = argparse.ArgumentParser(description='PowerCast Energy Harvesting Visualization')
    parser.add_argument('--sta-id', type=int, help='STA ID to analyze (if not provided, plots all STAs)')
    parser.add_argument('--csv-file', type=str, help='Path to CSV log file (default: ph-pcap/ph-harvester-demo-log.csv)')
    
    args = parser.parse_args()
    
    print("🔋 PowerCast Energy Harvesting Visualization")
    print("=" * 45)
    
    # Default CSV file path
    default_csv = 'ph-pcap/ph-harvester-demo-log.csv'
    
    # Get CSV file from command line argument or use default
    if args.csv_file:
        csv_file = args.csv_file
        if not os.path.exists(csv_file):
            print(f"❌ File '{csv_file}' not found.")
            return
        print(f"📁 Using CSV file (from argument): {csv_file}")
    else:
        # Use default file
        csv_file = default_csv
        if not os.path.exists(csv_file):
            print(f"❌ Default file '{csv_file}' not found.")
            print(f"💡 Please ensure the simulation has been run or specify a file with --csv-file")
            return
        print(f"📁 Using default CSV file: {csv_file}")
    
    # Load data
    
    try:
        # Read CSV with error handling for malformed lines
        df = pd.read_csv(csv_file, on_bad_lines='skip')
        print(f"✅ Loaded {len(df)} events from {csv_file}")
        
        # If that fails, try reading with different options
        if df.empty:
            print("⚠️  First attempt failed, trying with quoted fields...")
            df = pd.read_csv(csv_file, quotechar='"', skipinitialspace=True, on_bad_lines='skip')
            print(f"✅ Loaded {len(df)} events from {csv_file} (second attempt)")
            
    except Exception as e:
        print(f"❌ Error loading data: {e}")
        print("⚠️  Trying to load with error recovery...")
        try:
            # Try loading with more lenient parsing
            df = pd.read_csv(csv_file, error_bad_lines=False, warn_bad_lines=True)
            print(f"✅ Loaded {len(df)} events from {csv_file} (with some lines skipped)")
        except Exception as e2:
            print(f"❌ Final error: {e2}")
            return
    
    # Show available STAs
    available_stas = sorted(df['STA_ID'].unique())
    print(f"Available STAs: {available_stas}")
    
    # Load capacitor values from config file
    cap_values_from_config = get_capacitor_values_from_config()
    
    # Extract base name from CSV file for output naming
    csv_basename = os.path.splitext(os.path.basename(csv_file))[0]
    output_dir = os.path.dirname(csv_file) or 'ph-pcap'
    
    # Determine which STAs to process
    if args.sta_id is not None:
        # Single STA from command line
        sta_id = args.sta_id
        if sta_id not in available_stas:
            print(f"❌ STA {sta_id} not found. Available: {available_stas}")
            return
        stas_to_process = [sta_id]
        print(f"📊 Analyzing STA {sta_id} (from command line)")
    else:
        # Process all STAs
        stas_to_process = available_stas
        print(f"📊 Analyzing all {len(stas_to_process)} STAs")
    
    # Process each STA
    total_success = 0
    for sta_id in stas_to_process:
        print(f"\n{'='*60}")
        print(f"� Processing STA {sta_id}")
        print(f"{'='*60}")
    
        # Filter data for selected STA
        sta_data = df[df['STA_ID'] == sta_id].copy()
        
        if sta_data.empty:
            print(f"❌ No data found for STA {sta_id}")
            continue
        
        # Get STA configuration info
        voltage_class = sta_data['Volt_Class'].iloc[0] if not sta_data.empty else 1
        cap_class = sta_data['Cap_Class'].iloc[0] if not sta_data.empty else 'A'
        
        # Get capacitor description from config file
        cap_description = get_capacitor_description(cap_class, cap_values_from_config)
        
        volt_info = {
            '1': '1.2V Class (High Voltage)',
            '2': '0.9V Class (Medium Voltage)',
            '3': '0.7V Class (Low Voltage)'
        }
        
        volt_description = volt_info.get(str(voltage_class), f'Class {voltage_class} Voltage')
        
        # Print enhanced summary with configuration details
        print(f"\n📊 STA {sta_id} Configuration & Summary:")
        print(f"   🔋 Capacitor: {cap_description}")
        print(f"   ⚡ Voltage Threshold: {volt_description}")
        print(f"   📈 Total Events: {len(sta_data)}")
        print(f"   � RX Events: {len(sta_data[sta_data['Event_Type'] == 'RX'])}")
        print(f"   � TX Confirmed: {len(sta_data[sta_data['Event_Type'] == 'TX_CONFIRMED'])}")
        print(f"   🔴 TX Blocked: {len(sta_data[sta_data['Event_Type'] == 'TX_BLOCKED'])}")
        
        # Energy statistics
        total_harvested = sta_data['Harvested_Energy_J'].sum()
        total_consumed = sta_data['Consumed_Energy_J'].sum()
        net_energy = total_harvested - total_consumed
        print(f"   ⚡ Total Harvested: {total_harvested:.2e} J")
        print(f"   ⚡ Total Consumed: {total_consumed:.2e} J")
        print(f"   ⚡ Net Energy: {net_energy:.2e} J")
        
        # Create visualization
        print(f"\n🎨 Creating plot for STA {sta_id}...")
        fig = create_single_plot(sta_data, sta_id, voltage_class, cap_values_from_config)
        
        if fig is None:
            print("❌ Failed to create plot due to configuration errors.")
            continue
        
        # Generate filename: csv-basename_sta<id>_analysis.png
        filename = os.path.join(output_dir, f"{csv_basename}_sta{sta_id}_analysis.png")
        plt.savefig(filename, dpi=300, bbox_inches='tight', facecolor='white')
        print(f"✅ Plot saved to: {filename}")
        total_success += 1
        
        # Close figure to free memory
        plt.close(fig)
    
    # Final summary
    print(f"\n{'='*60}")
    print(f"✅ Analysis complete!")
    print(f"📊 Successfully processed {total_success}/{len(stas_to_process)} STAs")
    if total_success > 0:
        print(f"📁 Plots saved in: {output_dir}/")
        print(f"📝 Filename pattern: {csv_basename}_sta<ID>_analysis.png")
    print(f"{'='*60}")

if __name__ == "__main__":
    main()
