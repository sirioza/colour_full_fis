![photo_2025-11-15_20-58-32](https://github.com/user-attachments/assets/10ebe083-831b-4df1-a612-4b6c2aa6c215)

## Build modes

<img width="921" height="1136" alt="photo_2026-06-21_19-32-18" src="https://github.com/user-attachments/assets/d5847e3c-1834-46da-865d-b367f1320bf0" />

Default build uses the ESP32 built-in TWAI CAN controller on the pins configured in `config.h` and does not include mock CAN data in RAM.

For bench testing, create a local `local_config.h` file next to the `.ino` with:

```cpp
#define MOCK_CAN
```

`local_config.h` is ignored by git, so the mock mode stays local and will not affect the car build.
