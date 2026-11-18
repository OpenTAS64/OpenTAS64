# OpenTAS64 Plugins

OpenTAS64 currently provides an input bridge plugin for **Mupen64Plus 2.x**.

- [Mupen64Plus Network Input Plugin](mupen64plus-input/README.md) — loads as the emulator's Input plugin, receives TAS controller states from the OpenTAS64 desktop application over local TCP, and handles the emulator reset control command.

The desktop editor connects directly to the plugin at `127.0.0.1:6464`. No Mupen64Plus Launcher integration is required.
