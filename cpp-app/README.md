# Maintenance Log (Qt Windows App)

## Build (Windows)

### Requirements
- Qt 6 (Widgets + Network)
- CMake 3.16+
- Visual Studio Build Tools (MSVC)

### Build steps
```bash
cmake -S cpp-app -B build
cmake --build build --config Release
```

Executable output:
```
build/Release/MaintenanceLog.exe
```

## Prepare Windows redistributables
After building, collect Qt runtime files next to the executable:

```bash
<Qt6_Install_Path>/bin/windeployqt.exe build/Release/MaintenanceLog.exe
```

This will copy the required Qt DLLs and plugins beside the executable.

## Create installer (Inno Setup)
This repo ships an Inno Setup script at `installer/maintenance-log.iss`.

1. Install Inno Setup.
2. Update `MyAppVersion` if needed.
3. Run the script to produce a Windows installer.

The script expects the following layout:
```
build/Release/
  MaintenanceLog.exe
  Qt6*.dll
  platforms/
  styles/
```

If your Qt deployment folder is different, update `SourceDir` in the script.
