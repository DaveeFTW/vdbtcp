# VDBTCP - Vita TCP Debugger Companion

This plugin is a companion utility for the vita kernel-side debugger plugin: [kvdb][1]. You cannot use this application without that plugin already installed.

This plugin provides TCP/IP support for debugging applications on the vita. It also provides persistant FTP server and a [vitacompanion][2] compatible API.

If you are looking to debug your own application, you are probably looking to use this plugin.

## Install
To install copy this plugin to your device, and add it your `config.txt` under the `[main]` category.

## Usage
This provides three services:
* GDB stub on port 31337.
* FTP server on port 1337.
* vitacompanion service on port 1338.

This plugin can be used to launch an application in debug mode. To do this, you need to issue a `launch` command to the vitacompanion service on port 1338. This command differs from typical vitacompanion and has the following format:
```
launch /path/on/vita/to/self
```
Once you issue this command the vita will launch it, and the debugger will automatically attach and you can use the GDB protocol on port 31337 to debug it.

## Integrations into IDE
Coming soon: VSCode example.

tl;dr:
* Deploy application via FTP.
* Issue launch command to Vita.
* Attach to remote GDB process using IDE.

This method should work for most IDEs, but the specific integration will differ for each.

  [1]: https://github.com/DaveeFTW/kvdb
  [2]: https://github.com/devnoname120/vitacompanion