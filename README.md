# Hypr Control

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

A GTK4/Libadwaita control center for Hyprland input settings. Configure your mouse, keyboard, touchpad, and gestures with a modern, native UI.

## Features

### Input Configuration
- **Mouse**: Sensitivity, Acceleration Profile, Scroll Method, Left-handed mode.
- **Touchpad**: 
  - Toggle Touchpad/Touchscreen
  - Tapping (Tap to Click, Tap and Drag, Drag Lock)
  - Scrolling (Natural Scroll, Scroll Factor, Edge/Two-finger)
  - Click Behavior (Clickfinger, Middle Emulation)
- **Gestures**: Workspace Swipe (3/4 fingers), Distance, Invert, Continuous Swipe.
- **Keyboard**: 
  - Repeat Rate and Delay
  - **Layout Management**: View active layouts, add new ones.
  - **Layout Switching Keybind**: Manage your layout switching bind.

### Synchronization
The application automatically **syncs with your current Hyprland configuration** on startup.

## Installation

### Dependencies
- `gtk4`
- `libadwaita`
- `hyprland` (for `hyprctl`)
- `cmake` (build)
- `gcc` / `g++`

### Build
```bash
mkdir build
cd build
cmake ..
make
```

### Install
```bash
cp hypr-control ~/.local/bin/
cp ../hypr-control.desktop ~/.local/share/applications/
```

## Usage
Run `hypr-control` from your terminal or application launcher.
