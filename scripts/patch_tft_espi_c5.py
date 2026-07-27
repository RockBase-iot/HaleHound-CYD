Import("env")

from hashlib import sha256
from pathlib import Path
from urllib.request import urlopen


UPSTREAM_COMMIT = "67d80105c96edcfc949929a9e9adb620a52d6d00"
UPSTREAM_BASE = (
    "https://raw.githubusercontent.com/RockBase-iot/NM-CYD-C5/"
    f"{UPSTREAM_COMMIT}/Demos/Arduino/libraries/TFT_eSPI"
)
PROCESSOR_FILES = {
    "TFT_eSPI_ESP32_C5.h": "263a65f0c4efd5c3c45fa9186e08777d90833fd6bc52bd94b492194158c3bdfa",
    "TFT_eSPI_ESP32_C5.c": "59b8d83b418f3981bbe6a4c8b45b8407c4df61984e3025ca7055ccd64117d5bc",
}


def replace_once(path, old, new):
    content = path.read_text(encoding="utf-8")
    if new in content:
        return
    if content.count(old) != 1:
        raise RuntimeError(f"Unexpected TFT_eSPI 2.5.43 layout in {path}")
    path.write_text(content.replace(old, new), encoding="utf-8")


def install_processor_files(library_dir):
    processor_dir = library_dir / "Processors"
    processor_dir.mkdir(parents=True, exist_ok=True)

    for filename, expected_hash in PROCESSOR_FILES.items():
        destination = processor_dir / filename
        if destination.exists():
            current_hash = sha256(destination.read_bytes()).hexdigest()
            if current_hash == expected_hash:
                continue

        url = f"{UPSTREAM_BASE}/Processors/{filename}"
        with urlopen(url, timeout=30) as response:
            payload = response.read()

        actual_hash = sha256(payload).hexdigest()
        if actual_hash != expected_hash:
            raise RuntimeError(
                f"Checksum mismatch for {filename}: {actual_hash} != {expected_hash}"
            )
        destination.write_bytes(payload)


def patch_tft_espi(library_dir):
    install_processor_files(library_dir)

    replace_once(
        library_dir / "TFT_eSPI.h",
        '#elif defined(CONFIG_IDF_TARGET_ESP32C3)\n  #include "Processors/TFT_eSPI_ESP32_C3.h"\n#elif defined (ESP32)',
        '#elif defined(CONFIG_IDF_TARGET_ESP32C3)\n  #include "Processors/TFT_eSPI_ESP32_C3.h"\n#elif defined(CONFIG_IDF_TARGET_ESP32C5)\n  #include "Processors/TFT_eSPI_ESP32_C5.h"\n#elif defined (ESP32)',
    )
    replace_once(
        library_dir / "TFT_eSPI.cpp",
        '#elif defined(CONFIG_IDF_TARGET_ESP32C3)\n    #include "Processors/TFT_eSPI_ESP32_C3.c" // Tested with SPI (8-bit parallel will probably work too!)\n  #else',
        '#elif defined(CONFIG_IDF_TARGET_ESP32C3)\n    #include "Processors/TFT_eSPI_ESP32_C3.c" // Tested with SPI (8-bit parallel will probably work too!)\n  #elif defined(CONFIG_IDF_TARGET_ESP32C5)\n    #include "Processors/TFT_eSPI_ESP32_C5.c"\n  #else',
    )


if env.subst("$PIOENV") == "nm-cyd-c5":
    libdeps_dir = Path(env.subst("$PROJECT_LIBDEPS_DIR")) / env.subst("$PIOENV")
    tft_espi_dir = libdeps_dir / "TFT_eSPI"
    if not tft_espi_dir.is_dir():
        raise RuntimeError(f"TFT_eSPI dependency not found at {tft_espi_dir}")
    patch_tft_espi(tft_espi_dir)
    print(f"Patched TFT_eSPI 2.5.43 for ESP32-C5 from NM-CYD-C5 {UPSTREAM_COMMIT[:7]}")