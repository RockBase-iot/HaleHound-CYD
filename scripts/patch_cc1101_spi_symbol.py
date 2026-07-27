Import("env")

from pathlib import Path


def replace_exact(content, old, new, expected_count):
    actual_count = content.count(old)
    if actual_count == 0 and new in content:
        return content, False
    if actual_count != expected_count:
        raise RuntimeError(
            f"Unexpected SmartRC-CC1101 2.5.7 layout for {old!r}: {actual_count} matches"
        )
    return content.replace(old, new), True


library_dir = Path(env.subst("$PROJECT_LIBDEPS_DIR")) / env.subst("$PIOENV")
source = library_dir / "SmartRC-CC1101-Driver-Lib" / "ELECHOUSE_CC1101_SRC_DRV.cpp"
if not source.exists():
    raise RuntimeError(f"SmartRC-CC1101 dependency not found at {source}")

content = source.read_text(encoding="utf-8")
changed = False
for old, new, count in (
    ("bool spi = 0;", "bool cc1101SpiPinsConfigured = false;", 1),
    ("if (spi == 0){", "if (!cc1101SpiPinsConfigured){", 1),
    ("spi = 1;", "cc1101SpiPinsConfigured = true;", 2),
):
    content, replacement_changed = replace_exact(content, old, new, count)
    changed = changed or replacement_changed

if changed:
    source.write_text(content, encoding="utf-8", newline="\n")

print("Patched SmartRC-CC1101 2.5.7 global SPI state symbol")