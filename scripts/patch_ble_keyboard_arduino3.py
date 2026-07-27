Import("env")

from pathlib import Path


def patch_ble_keyboard():
    library_dir = Path(env.subst("$PROJECT_LIBDEPS_DIR")) / env.subst("$PIOENV")
    source = library_dir / "ESP32 BLE Keyboard" / "BleKeyboard.cpp"

    if not source.exists():
        raise RuntimeError(f"ESP32 BLE Keyboard dependency not found at {source}")

    content = source.read_text(encoding="utf-8")
    replacements = {
        "BLEDevice::init(deviceName);":
            "BLEDevice::init(String(deviceName.c_str()));",
        "hid->manufacturer()->setValue(deviceManufacturer);":
            "hid->manufacturer()->setValue(String(deviceManufacturer.c_str()));",
    }

    changed = False
    for old, new in replacements.items():
        if new in content:
            continue
        if old not in content:
            raise RuntimeError(
                f"ESP32 BLE Keyboard source changed; expected line not found: {old}"
            )
        content = content.replace(old, new, 1)
        changed = True

    if changed:
        source.write_text(content, encoding="utf-8", newline="\n")


if env.subst("$PIOENV") == "nm-cyd-c5":
    patch_ble_keyboard()
    print("Patched ESP32 BLE Keyboard 0.3.2 for Arduino ESP32 3.x String APIs")