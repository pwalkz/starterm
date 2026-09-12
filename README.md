# starterm

Starterm - Terminal Emulator

A lightweight terminal emulator built with Qt and QTermWidget. Features: Multiple tabs Copy & Paste support Zoom in/out Multiple color themes Keyboard shortcuts Remembers your theme preference Built with Qt 5/6 and QTermWidget Peter Walker © 2026 Starterm Project

It is optimised for Linux xfce4 and qt6 widget set. It is coded and compiled with cpp.

Dependency install and checks:
- sudo pacman -Syu qtermwidget
- Then check version: pacman -Q qt6-base
- Check what version of QTermWidget you have: pacman -Q qtermwidget
- Check available versions in AUR: yay -Ss qtermwidget

Color Schemes check and optional add:
- sudo cp Borland.colorscheme /usr/share/qtermwidget6/color-schemes

