## Build modes

Default build uses the ESP32 built-in TWAI CAN controller on the pins configured in `config.h` and does not include mock CAN data in RAM.

For bench testing, create a local `local_config.h` file next to the `.ino` with:

```cpp
#define MOCK_CAN
```

`local_config.h` is ignored by git, so the mock mode stays local and will not affect the car build.
