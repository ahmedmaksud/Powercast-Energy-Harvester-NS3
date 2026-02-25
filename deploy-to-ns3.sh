#!/bin/bash

# PowerCast Energy Harvester - NS-3 Setup and Run Script
# Sets up directories, builds, runs simulation, and generates plots

set -e

# Get script directory (powercast_hardware)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Navigate to ns-3 root (4 levels up from powercast_hardware)
NS3_ROOT="$(cd "$SCRIPT_DIR/../../../.." && pwd)"
EXAMPLES_DIR="$SCRIPT_DIR/.."

# Verify we're in the right place
if [ ! -f "$NS3_ROOT/ns3" ]; then
    echo "Error: Cannot find ns3 executable at $NS3_ROOT/ns3"
    echo "Expected directory structure: ns-3.44/contrib/ai/examples/powercast_hardware/"
    exit 1
fi

echo "PowerCast Hardware Setup & Run"
echo "=============================="
echo "Script directory: $SCRIPT_DIR"
echo "NS-3 root: $NS3_ROOT"
echo ""

# Create output directory for PCAP and CSV files
mkdir -p "$SCRIPT_DIR/ph-pcap"
echo "✓ Created output directory: ph-pcap/"

# Add to CMakeLists.txt if not present
CMAKE_FILE="$EXAMPLES_DIR/CMakeLists.txt"
if [ -f "$CMAKE_FILE" ]; then
    if ! grep -q "powercast_hardware" "$CMAKE_FILE" 2>/dev/null; then
        echo "add_subdirectory(powercast_hardware)" >> "$CMAKE_FILE"
        echo "✓ Added powercast_hardware to examples/CMakeLists.txt"
    else
        echo "✓ powercast_hardware already in CMakeLists.txt"
    fi
else
    echo "⚠ Warning: $CMAKE_FILE not found"
fi

# Build NS-3
echo ""
echo "Building NS-3..."
cd "$NS3_ROOT"
./ns3 build ph-harvester-demo

# Run the harvester demo
echo ""
echo "Running PowerCast Harvester Demo..."
./ns3 run ph-harvester-demo

# Activate venv and run plotter
echo ""
echo "Generating plots..."
cd "$SCRIPT_DIR"

# Try to find and activate virtual environment
VENV_PATHS=(
    "$NS3_ROOT/../EHRL/bin/activate"
    "$NS3_ROOT/../../EHRL/bin/activate"
    "$HOME/NS3-project/EHRL/bin/activate"
    "$SCRIPT_DIR/venv/bin/activate"
)

VENV_ACTIVATED=false
for venv_path in "${VENV_PATHS[@]}"; do
    if [ -f "$venv_path" ]; then
        echo "✓ Activating venv: $venv_path"
        source "$venv_path"
        VENV_ACTIVATED=true
        break
    fi
done

if [ "$VENV_ACTIVATED" = false ]; then
    echo "⚠ No venv found, using system Python"
fi

# Run plotter
if [ -f "$SCRIPT_DIR/ph-harvester-plot.py" ]; then
    python3 "$SCRIPT_DIR/ph-harvester-plot.py"
    echo "✓ Plots generated"
else
    echo "⚠ Plotter script not found: ph-harvester-plot.py"
fi

echo ""
echo "Done!"
