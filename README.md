# BMA423 Accelerometer Module

Zephyr module for the Bosch BMA423 triaxial accelerometer on the Watchy smartwatch. Uses the out-of-tree Bosch BMA423 C library directly over Zephyr's I2C API.

## Features

- 3-axis accelerometer (configured at 100 Hz, +/-2g, continuous mode)
- Step counter (pedometer) with reset capability
- Activity recognition (still / walking / running)
- Temperature reading (milli-degrees Celsius)
- Tilt-on-wrist and double-tap (wakeup) detection
- Interrupt mapping to INT1
- Zbus request-response integration for on-demand sensor reads
- Shell commands for direct sensor access

## Directory Structure

```
bma423/
├── CMakeLists.txt
├── Kconfig
├── mainBma423.c              # Thread entry: init sequence + message loop
├── drvr/
│   ├── inc/
│   │   ├── bma4.h             # Bosch BMA4 base API
│   │   ├── bma4_defs.h        # Bosch register/type definitions
│   │   ├── bma423.h           # Bosch BMA423 feature API
│   │   ├── bma.h              # I2C read/write helpers
│   │   ├── msgqBma423.h       # Module message queue types
│   │   ├── wdtBma423.h        # Watchdog timer interface
│   │   ├── zbusBma423.h       # Zbus listener interface
│   │   └── zephyrBma423.h     # bmaDevGet() — Zephyr I2C bridge
│   └── src/
│       ├── bma4.c             # Bosch BMA4 base driver
│       ├── bma423.c           # Bosch BMA423 feature driver
│       ├── bma.cpp            # I2C transport (read/write via Zephyr)
│       ├── msgqBma423.c       # Message handlers (sample, reset steps)
│       ├── wdtBma423.c        # Watchdog feed
│       ├── zbusBma423.c       # Zbus listener (REQ → msgq forwarding)
│       └── zephyrBma423.c     # bmaDevGet() impl (I2C bus + address)
├── dts/bindings/              # Devicetree bindings
├── shell/src/
│   └── shellBma423.c          # Shell commands (direct BMA423 calls)
└── host/
    └── hostBma423Test.sh      # Host-side test automation script
```

## Architecture

### Thread Model

The module runs in its own Zephyr thread (`bma423_main_id`). After hardware initialization, the thread blocks on its internal message queue waiting for work:

```
mainBma423.c
  └─ bma423Main()
       ├─ Hardware init (soft reset, config file, accel config, features)
       └─ Loop: wdtFeed() + msgqBma423Proc()
```

### Zbus Communication

Follows the same request-response pattern as the ADC module. All channels are guarded by `CONFIG_BMA423`.

| Channel | Direction | Purpose |
|---|---|---|
| `ZBUS_CHAN_BMA423_SAMPLE_REQ` | App -> Module | Request sensor data |
| `ZBUS_CHAN_BMA423_SAMPLE_SUB` | Module -> App | Publish sensor data |
| `ZBUS_CHAN_BMA423_RESET_STEPS` | App -> Module | Reset step counter |

**Message struct** (`ZbusMsgBma423`):
```c
typedef struct {
    int16_t accelX;       // Accelerometer X-axis
    int16_t accelY;       // Accelerometer Y-axis
    int16_t accelZ;       // Accelerometer Z-axis
    uint32_t steps;       // Step counter value
    uint8_t activity;     // 0=still, 1=walking, 2=running
    int32_t tempMilli;    // Temperature in milli-degrees Celsius
} ZbusMsgBma423;
```

### Data Flow

```
App calls zbusBma423SampleReq()
  → publishes to ZBUS_CHAN_BMA423_SAMPLE_REQ
    → zbusBma423.c listener forwards to module msgq
      → msgqBma423.c reads all sensors via Bosch API
        → publishes result to ZBUS_CHAN_BMA423_SAMPLE_SUB
          → App reads response
```

### App-Side Helpers (`zbusMain.h`)

```c
// Synchronous request-response: requests sensor data, blocks up to 1s
int zbusBma423SampleReq(ZbusMsgBma423 *result);

// Fire-and-forget: publishes reset command
int zbusBma423ResetSteps(void);
```

## Kconfig Options

| Option | Default | Description |
|---|---|---|
| `CONFIG_BMA423` | n | Enable BMA423 module (requires `I2C`) |
| `CONFIG_BMA423_STEP_COUNTER` | y | Enable step counter feature |
| `CONFIG_BMA423_ACTIVITY_RECOGNITION` | y | Enable activity recognition |
| `CONFIG_BMA423_TILT_DETECTION` | n | Enable tilt-on-wrist detection |
| `CONFIG_BMA423_TRIGGER` | y | Enable interrupt support (requires `GPIO`) |
| `CONFIG_BMA423_SHELL` | n | Enable shell commands |
| `CONFIG_BMA423_PRIORITY` | 3 | Thread priority |
| `CONFIG_BMA423_STACK_SIZE` | 1024 | Thread stack size |
| `CONFIG_BMA423_MSGQ_SIZE` | 5 | Message queue depth |
| `CONFIG_BMA423_MSGQ_TIMEOUT_S` | 1 | Message wait timeout (seconds) |
| `CONFIG_BMA423_WDT_TIMEOUT_S` | 2 | Watchdog timeout (seconds) |

## Shell Commands

Enabled with `CONFIG_BMA423_SHELL=y`. Commands call Bosch API functions directly (not through Zbus).

```
uart:~$ bma423 accel          # Read accelerometer XYZ
uart:~$ bma423 steps          # Read step counter
uart:~$ bma423 activity       # Read activity (still/walking/running)
uart:~$ bma423 temp           # Read temperature in Celsius
uart:~$ bma423 reset_steps    # Reset step counter to zero
```

## Hardware Configuration

The BMA423 is connected via I2C at address `0x18` (SDO tied to GND). Devicetree overlay:

```dts
&i2c0 {
    bma423: i2c_device@18 {
        reg = <0x18>;
    };
};
```

The module accesses the I2C bus via `bmaDevGet()` which returns a `struct bma4_dev*` context bridged to Zephyr's I2C driver using `DEVICE_DT_GET(DT_BUS(bma423_node))`.

## Initialization Sequence

1. Soft reset (`0xB6` to register `0x7E`) + 20 ms delay
2. `bma423_init()` — chip ID verification, disable advanced power save
3. `bma423_write_config_file()` — load 6144-byte feature engine config
4. Configure INT1 pin (level trigger, active high, push-pull)
5. Set accelerometer config (100 Hz, +/-2g, normal bandwidth, continuous mode)
6. Enable accelerometer
7. Remap axes (X/Y swapped with sign inversion for watch orientation)
8. Enable features: step counter, tilt, wakeup (double-tap)
9. Reset step counter
10. Map feature interrupts to INT1
