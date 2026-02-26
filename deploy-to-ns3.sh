#!/bin/bash
#
# Author: Ahmed Maksud; email: ahmed.maksud@email.ucr.edu
# PI: Marcelo Menezes De Carvalho; email: mmcarvalho@txstate.edu
# Texas State University
#
# PowerCast Energy Harvester - NS-3 Setup and Run Script
# ======================================================
# This script automates the complete workflow for running the PowerCast energy
# harvesting simulation:
# 1. Deploy source files to NS3 contrib directory
# 2. Update CMakeLists.txt if needed
# 3. Build the ph-harvester-demo executable
# 4. Run the simulation
# 5. Setup Python virtual environment and generate plots
#
# Usage: ./deploy-to-ns3.sh (from Powercast-Energy-Harvester-NS3 directory)
# Deploys to: ~/NS3-project/ns-allinone-3.44/ns-3.44/contrib/ai/examples/powercast_hardware/

set -e  # Exit immediately if any command fails

# Source directory (where this script lives)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# NS-3 paths
NS3_ROOT="$HOME/NS3-project/ns-allinone-3.44/ns-3.44"
EXAMPLES_DIR="$NS3_ROOT/contrib/ai/examples"
DEPLOY_DIR="$EXAMPLES_DIR/powercast_hardware"

# Verify NS-3 exists
if [ ! -f "$NS3_ROOT/ns3" ]; then
	echo "Error: Cannot find ns3 executable at $NS3_ROOT/ns3"
	echo "Expected NS-3 installation at: $NS3_ROOT"
	exit 1
fi

echo "PowerCast Hardware Deploy & Run"
echo "================================"
echo "Source directory: $SCRIPT_DIR"
echo "NS-3 root: $NS3_ROOT"
echo "Deploy to: $DEPLOY_DIR"
echo ""

# Create deployment directory
mkdir -p "$DEPLOY_DIR"
echo "✓ Created deployment directory: $DEPLOY_DIR"

# Copy source files to deployment directory
echo "Copying files to NS-3..."
cp -v "$SCRIPT_DIR/CMakeLists.txt" "$DEPLOY_DIR/"
cp -v "$SCRIPT_DIR/ph-deployment-helper.cc" "$DEPLOY_DIR/"
cp -v "$SCRIPT_DIR/ph-deployment-helper.h" "$DEPLOY_DIR/"
cp -v "$SCRIPT_DIR/ph-harvester-demo.cc" "$DEPLOY_DIR/"
cp -v "$SCRIPT_DIR/ph-harvester-hardware.cc" "$DEPLOY_DIR/"
cp -v "$SCRIPT_DIR/ph-harvester-hardware.h" "$DEPLOY_DIR/"
cp -v "$SCRIPT_DIR/ph-harvester-config.txt" "$DEPLOY_DIR/"
cp -v "$SCRIPT_DIR/ph-harvester-plot.py" "$DEPLOY_DIR/"
echo "✓ Files copied"

# Create output directory for PCAP and CSV files
mkdir -p "$DEPLOY_DIR/ph-pcap"
echo "✓ Created output directory: $DEPLOY_DIR/ph-pcap/"

# Add to CMakeLists.txt if not present
CMAKE_FILE="$EXAMPLES_DIR/CMakeLists.txt"
if [ -f "$CMAKE_FILE" ]; then
	if ! grep -q "powercast_hardware" "$CMAKE_FILE" 2>/dev/null; then
		echo "add_subdirectory(powercast_hardware)" >>"$CMAKE_FILE"
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
cd "$DEPLOY_DIR"

# Try to find and activate virtual environment
VENV_PATHS=(
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
if [ -f "$DEPLOY_DIR/ph-harvester-plot.py" ]; then
	python3 "$DEPLOY_DIR/ph-harvester-plot.py"
	echo "✓ Plots generated"
else
	echo "⚠ Plotter script not found: ph-harvester-plot.py"
fi

echo ""
echo "Done!"
