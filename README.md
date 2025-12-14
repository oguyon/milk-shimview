# milkshmimview

A high-performance GTK4 viewer for `ImageStreamIO` shared memory image streams, designed for scientific and real-time monitoring applications.

## Features

*   **Real-time Display:** High frame-rate visualization of shared memory streams.
*   **Interactive Interface:**
    *   **Pan:** Left-click drag (when zoomed).
    *   **ROI Selection:** Shift + Left-click drag.
    *   **Contrast/Bias:** Right-click drag.
    *   **Zoom:** Mouse scroll or Fixed/Fit modes.
*   **Advanced Analysis:**
    *   **Statistics:** Real-time Min, Max, Mean, Median, 10th/90th Percentile, Pixel Count, and Sum.
    *   **Histogram:** Interactive Linear/Log histogram with overlay cursor inspection and CDF/Inverse CDF curves.
    *   **Trace Plot:** Time-series "waterfall" heatmap visualization of statistics with gap-filling and synchronized coloring.
*   **Flexible Scaling:**
    *   **Auto Scale:** Toggle with memory (restores previous auto settings). Modes: Min/Max, Percentiles (1%, 99%, etc.).
    *   **Manual Scale:** Direct control over min/max values.
    *   **Thresholds:** Visual masking (Red/Blue) for values outside user-defined limits.
    *   **Colormaps:** Standard scientific colormaps (Inferno, Viridis-like, Cool, Heat, Rainbow, etc.) starting at black.
*   **Layout:**
    *   3-pane design (Controls, Image, Stats) with resizeable splitters.
    *   Collapsible sidebars for maximum screen real estate.

## Prerequisites

*   **GTK4**: `libgtk-4-dev`
*   **ImageStreamIO**: Must be installed or available in include/link paths.
*   **CMake**: Build system.

## Build

```bash
mkdir -p viewer/build
cd viewer/build
cmake ..
make
```

## Usage

### 1. Start a Stream (Simulation)

A test program `earth_stream` is included to generate a 256x256 @ 100Hz floating-point stream with moving patterns.

```bash
# Set shared memory directory
export MILK_SHM_DIR=/tmp/milk/shm
mkdir -p $MILK_SHM_DIR

# Run generator
./earth_stream &
```

### 2. Run Viewer

```bash
# Run viewer connecting to stream "earth"
./milkshmimview earth
```

### Controls

| Action | Input | Description |
| :--- | :--- | :--- |
| **Pan** | Left Drag | Move the image when zoomed in. |
| **ROI** | Shift + Left Drag | Draw or move a Region of Interest. |
| **Contrast** | Right Drag | Adjust contrast (width) and bias (center) of the colormap. |
| **Inspect** | Hover | View pixel coordinates and values under cursor. |
| **Zoom** | Scroll | Zoom in/out (centered on cursor in Fixed mode). |

### UI Overview

*   **Left Panel:** Global controls for FPS, Colormap, Scaling (Linear/Log/Sqrt), and Auto Scale settings.
    *   **Auto Scale Button:** Toggles between Manual mode and the last used Auto configuration.
    *   **Thresholds:** Set Min/Max limits to force pixels to Blue/Red respectively.
*   **Center:** Main image display.
*   **Right Panel:**
    *   **Colorbar:** Interactive scale; hover to inspect values.
    *   **Stats:** Detailed statistics for the full frame or selected ROI.
    *   **Trace:** Time history of stats. Use `+`/`-` to adjust duration (1.2x steps).

## Screenshots

*(Place screenshots here)*

![Main View](docs/screenshot_main.png)
