# Secure Capture

Secure Capture helps control which application windows may be included in
screenshots and screen recordings on KDE Plasma 6.

## Abilities

- Capture protection tray icon.
- Allow selected applications to remain capturable.
- Use `*` in application patterns to match any sequence of characters.
- Apply allowlist changes to existing and newly opened windows.
- Configure automatic activation rules for selected applications.
- Restore the manual state after the last matching trigger window closes.
- Optionally include the desktop, Plasma panels, and Tab Switcher separately.
- Restore modified windows when protection is disabled.

Patterns match the complete application identifier. Only `*` has special
meaning; all other characters are treated literally.

## Installation

Install the required KDE, Qt, and build packages for your distribution,
including:

- CMake
- Extra CMake Modules
- Qt 6
- KDE Frameworks 6
- Plasma 6 and KWin development files

On most distributions, build and install with:

```sh
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
sudo cmake --install build
```

## Setup

1. Open **System Settings**.
2. Go to **Window Management > Desktop Effects**.
3. Enable **Secure Capture**.
4. Open the **System Tray** configuration.
5. Add **Secure Capture** to the visible tray items.

## Usage

Click the tray icon to:

- Toggle capture protection.
- Choose separately whether the desktop, Plasma panels, and Tab Switcher are included.
- View whether protection is enabled manually or automatically.
- Add or remove allowlist patterns.
- Add or remove automatic activation rules.

An automatic rule consists of an application pattern and an action:

- **Enable**: activate protection while a matching window exists.
- **Disable**: deactivate protection while a matching window exists.

If matching enable and disable rules are active at the same time, the disable action takes precedence.
