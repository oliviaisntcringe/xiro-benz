# triada.benz injector

Build `injector/injector.vcxproj` as `Development|x64` or `Release|x64`. The output is written to `bin/triada-injector-debug.exe` or `bin/triada-injector.exe`.

Running the executable with no options detects the configured CS2 installation, reuses an existing `cs2.exe` when present, waits for `client.dll` and `engine2.dll`, and injects the nearest `velocity-debug.dll` (falling back to `velocity.dll`). With no `cs2.exe` running it launches the game using `-insecure -novid`.

Examples:

```text
bin\triada-injector-debug.exe
bin\triada-injector-debug.exe --launch --dll bin\velocity-debug.dll
bin\triada-injector-debug.exe --pid 12345 --log runlogs\injector.log
bin\triada-injector-debug.exe --args "-insecure -novid -windowed -w 1280 -h 720"
```

The injector only accepts a target whose image name is `cs2.exe`. Every launch, module wait, remote allocation, thread, and result is printed to the console and written to `injector.log` unless `--log` overrides it.
