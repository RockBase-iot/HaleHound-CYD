#include "features/ble/GattNameTable.h"

#include <cstddef>

namespace {

struct NamedUuid {
    uint16_t uuid;
    const char* name;
};

// Bluetooth SIG Assigned Numbers -> GATT Services.
// Vendor entries are the 0xFFxx range and say so in the name.
constexpr NamedUuid kServices[] = {
    {0x1800, "GAP"},
    {0x1801, "GATT"},
    {0x1802, "Immediate Alert"},
    {0x1803, "Link Loss"},
    {0x1804, "Tx Power"},
    {0x1805, "Current Time"},
    {0x1806, "Reference Time Update"},
    {0x1807, "Next DST Change"},
    {0x1808, "Glucose"},
    {0x1809, "Health Thermometer"},
    {0x180A, "Device Information"},
    {0x180D, "Heart Rate"},
    {0x180E, "Phone Alert Status"},
    {0x180F, "Battery"},
    {0x1810, "Blood Pressure"},
    {0x1811, "Alert Notification"},
    {0x1812, "Human Interface Device"},
    {0x1813, "Scan Parameters"},
    {0x1814, "Running Speed and Cadence"},
    {0x1815, "Automation IO"},
    {0x1816, "Cycling Speed and Cadence"},
    {0x1818, "Cycling Power"},
    {0x1819, "Location and Navigation"},
    {0x181A, "Environmental Sensing"},
    {0x181B, "Body Composition"},
    {0x181C, "User Data"},
    {0x181D, "Weight Scale"},
    {0x181E, "Bond Management"},
    {0x181F, "Continuous Glucose Monitoring"},
    {0x1820, "Internet Protocol Support"},
    {0x1821, "Indoor Positioning"},
    {0x1822, "Pulse Oximeter"},
    {0x1823, "HTTP Proxy"},
    {0x1824, "Transport Discovery"},
    {0x1825, "Object Transfer"},
    {0x1826, "Fitness Machine"},
    {0x1827, "Mesh Provisioning"},
    {0x1828, "Mesh Proxy"},
    {0x1829, "Reconnection Configuration"},
    {0x183A, "Insulin Delivery"},
    {0x183B, "Binary Sensor"},
    {0x183C, "Emergency Configuration"},
    {0x183E, "Physical Activity Monitor"},
    {0x1843, "Audio Input Control"},
    {0x1844, "Volume Control"},
    {0x1845, "Volume Offset Control"},
    {0x1846, "Coordinated Set Identification"},
    {0x1847, "Device Time"},
    {0x1848, "Media Control"},
    {0x1849, "Generic Media Control"},
    {0x184A, "Constant Tone Extension"},
    {0x184B, "Telephone Bearer"},
    {0x184C, "Generic Telephone Bearer"},
    {0x184D, "Microphone Control"},
    {0x184E, "Audio Stream Control"},
    {0x184F, "Broadcast Audio Scan"},
    {0x1850, "Published Audio Capabilities"},
    {0x1851, "Basic Audio Announcement"},
    {0x1852, "Broadcast Audio Announcement"},
    {0x1853, "Common Audio"},
    {0x1854, "Hearing Access"},
    {0x1855, "Telephony and Media Audio"},
    {0x1856, "Public Broadcast Announcement"},
    {0x1857, "Electronic Shelf Label"},
    {0x1858, "Gaming Audio"},
    {0x1859, "Mesh Proxy Solicitation"},
    {0xFD0D, "Blecon service (member)"},
    {0xFD6F, "Exposure Notification Service (member)"},
    {0xFE0F, "Philips Hue service (member)"},
    {0xFE2C, "Fast Pair service (member)"},
    {0xFE59, "Nordic Secure DFU Service (vendor)"},
    {0xFEAA, "Eddystone beacon (member)"},
    {0xFEBB, "Adafruit service (member)"},
    {0xFFE0, "HM-10 / UART bridge (vendor)"},
    {0xFFF0, "Triones / ELK-BLEDOM LED service (vendor)"},
};

// Bluetooth SIG Assigned Numbers -> GATT Characteristics.
constexpr NamedUuid kCharacteristics[] = {
    {0x2A00, "Device Name"},
    {0x2A01, "Appearance"},
    {0x2A02, "Peripheral Privacy Flag"},
    {0x2A03, "Reconnection Address"},
    {0x2A04, "Peripheral Preferred Connection Parameters"},
    {0x2A05, "Service Changed"},
    {0x2A06, "Alert Level"},
    {0x2A07, "Tx Power Level"},
    {0x2A08, "Date Time"},
    {0x2A09, "Day of Week"},
    {0x2A0A, "Day Date Time"},
    {0x2A0C, "Exact Time 256"},
    {0x2A0D, "DST Offset"},
    {0x2A0E, "Time Zone"},
    {0x2A0F, "Local Time Information"},
    {0x2A11, "Time with DST"},
    {0x2A12, "Time Accuracy"},
    {0x2A13, "Time Source"},
    {0x2A14, "Reference Time Information"},
    {0x2A16, "Time Update Control Point"},
    {0x2A17, "Time Update State"},
    {0x2A18, "Glucose Measurement"},
    {0x2A19, "Battery Level"},
    {0x2A1C, "Temperature Measurement"},
    {0x2A1D, "Temperature Type"},
    {0x2A1E, "Intermediate Temperature"},
    {0x2A21, "Measurement Interval"},
    {0x2A22, "Boot Keyboard Input Report"},
    {0x2A23, "System ID"},
    {0x2A24, "Model Number String"},
    {0x2A25, "Serial Number String"},
    {0x2A26, "Firmware Revision String"},
    {0x2A27, "Hardware Revision String"},
    {0x2A28, "Software Revision String"},
    {0x2A29, "Manufacturer Name String"},
    {0x2A2A, "IEEE 11073-20601 Regulatory Certification Data List"},
    {0x2A2B, "Current Time"},
    {0x2A2C, "Magnetic Declination"},
    {0x2A31, "Scan Refresh"},
    {0x2A35, "Blood Pressure Measurement"},
    {0x2A36, "Intermediate Cuff Pressure"},
    {0x2A37, "Heart Rate Measurement"},
    {0x2A38, "Body Sensor Location"},
    {0x2A39, "Heart Rate Control Point"},
    {0x2A3F, "Alert Status"},
    {0x2A40, "Ringer Control Point"},
    {0x2A41, "Ringer Setting"},
    {0x2A42, "Alert Category ID Bit Mask"},
    {0x2A43, "Alert Category ID"},
    {0x2A44, "Alert Notification Control Point"},
    {0x2A45, "Unread Alert Status"},
    {0x2A46, "New Alert"},
    {0x2A47, "Supported New Alert Category"},
    {0x2A48, "Supported Unread Alert Category"},
    {0x2A49, "Blood Pressure Feature"},
    {0x2A4A, "HID Information"},
    {0x2A4B, "Report Map"},
    {0x2A4C, "HID Control Point"},
    {0x2A4D, "Report"},
    {0x2A4E, "Protocol Mode"},
    {0x2A4F, "Scan Interval Window"},
    {0x2A50, "PnP ID"},
    {0x2A51, "Glucose Feature"},
    {0x2A52, "Record Access Control Point"},
    {0x2A53, "RSC Measurement"},
    {0x2A54, "RSC Feature"},
    {0x2A55, "SC Control Point"},
    {0x2A5A, "Aggregate"},
    {0x2A5B, "CSC Measurement"},
    {0x2A5C, "CSC Feature"},
    {0x2A5D, "Sensor Location"},
    {0x2A5E, "PLX Spot-Check Measurement"},
    {0x2A5F, "PLX Continuous Measurement"},
    {0x2A60, "PLX Features"},
    {0x2A63, "Cycling Power Measurement"},
    {0x2A65, "Cycling Power Feature"},
    {0x2A66, "Cycling Power Control Point"},
    {0x2A67, "Location and Speed"},
    {0x2A68, "Navigation"},
    {0x2A69, "Position Quality"},
    {0x2A6A, "LN Feature"},
    {0x2A6B, "LN Control Point"},
    {0x2A6C, "Elevation"},
    {0x2A6D, "Pressure"},
    {0x2A6E, "Temperature"},
    {0x2A6F, "Humidity"},
    {0x2A70, "True Wind Speed"},
    {0x2A71, "True Wind Direction"},
    {0x2A72, "Apparent Wind Speed"},
    {0x2A73, "Apparent Wind Direction"},
    {0x2A74, "Gust Factor"},
    {0x2A75, "Pollen Concentration"},
    {0x2A76, "UV Index"},
    {0x2A77, "Irradiance"},
    {0x2A78, "Rainfall"},
    {0x2A79, "Wind Chill"},
    {0x2A7A, "Heat Index"},
    {0x2A7B, "Dew Point"},
    {0x2A98, "Weight"},
    {0x2A9D, "Weight Measurement"},
    {0x2A9E, "Weight Scale Feature"},
    {0x2A9F, "User Control Point"},
    {0x2AA6, "Central Address Resolution"},
    {0xFFE1, "HM-10 / UART bridge data (vendor)"},
    {0xFFF3, "Triones / ELK-BLEDOM LED control (vendor)"},
    {0xFFF4, "Triones / ELK-BLEDOM LED notify (vendor)"},
};

// Bluetooth SIG Assigned Numbers -> GATT Descriptors.
constexpr NamedUuid kDescriptors[] = {
    {0x2900, "Characteristic Extended Properties"},
    {0x2901, "Characteristic User Description"},
    {0x2902, "Client Characteristic Configuration"},
    {0x2903, "Server Characteristic Configuration"},
    {0x2904, "Characteristic Presentation Format"},
    {0x2905, "Characteristic Aggregate Format"},
    {0x2906, "Valid Range"},
    {0x2907, "External Report Reference"},
    {0x2908, "Report Reference"},
    {0x2909, "Number of Digitals"},
    {0x290A, "Value Trigger Setting"},
    {0x290B, "Environmental Sensing Configuration"},
    {0x290C, "Environmental Sensing Measurement"},
    {0x290D, "Environmental Sensing Trigger Setting"},
    {0x290E, "Time Trigger Setting"},
};

// The low 96 bits of every SIG-assigned UUID, as nibbles. `0000-1000-8000-
// 00805f9b34fb` -> the trailing 24 hex digits.
constexpr uint8_t kBaseUuidLowNibbles[24] = {
    0x0, 0x0, 0x0, 0x0,  // second field: 0000
    0x1, 0x0, 0x0, 0x0,  // third field:  1000
    0x8, 0x0, 0x0, 0x0,  // fourth field: 8000
    0x0, 0x0, 0x8, 0x0,  // fifth field:  0080
    0x5, 0xF, 0x9, 0xB,  //              5f9b
    0x3, 0x4, 0xF, 0xB,  //              34fb
};

// Nibble value of a hex digit, or 0xFF when the character is not hex.
uint8_t hexNibble(char c) {
    if (c >= '0' && c <= '9') {
        return static_cast<uint8_t>(c - '0');
    }
    if (c >= 'a' && c <= 'f') {
        return static_cast<uint8_t>(c - 'a' + 10);
    }
    if (c >= 'A' && c <= 'F') {
        return static_cast<uint8_t>(c - 'A' + 10);
    }
    return 0xFF;
}

uint16_t nibblesToUint16(const uint8_t* nibbles) {
    return static_cast<uint16_t>((static_cast<uint16_t>(nibbles[0]) << 12) |
                                 (static_cast<uint16_t>(nibbles[1]) << 8) |
                                 (static_cast<uint16_t>(nibbles[2]) << 4) |
                                 static_cast<uint16_t>(nibbles[3]));
}

const NamedUuid* lookup(const NamedUuid* table, size_t count, uint16_t uuid) {
    for (size_t index = 0; index < count; ++index) {
        if (table[index].uuid == uuid) {
            return &table[index];
        }
    }
    return nullptr;
}

bool strictlyAscending(const NamedUuid* table, size_t count) {
    for (size_t index = 1; index < count; ++index) {
        // `<=` rather than `<`: equal keys would mean a duplicate, and lookup()
        // would then hide the second one forever.
        if (table[index].uuid <= table[index - 1].uuid) {
            return false;
        }
    }
    return true;
}

}  // namespace

const char* GattNameTable::baseUuidSuffix() {
    return "0000-1000-8000-00805f9b34fb";
}

bool GattNameTable::toShortUuid(const std::string& uuid, uint16_t& out) {
    uint8_t nibbles[32];
    size_t count = 0;

    size_t start = 0;
    if (uuid.size() >= 2 && uuid[0] == '0' && (uuid[1] == 'x' || uuid[1] == 'X')) {
        start = 2;
    }
    for (size_t index = start; index < uuid.size(); ++index) {
        const char c = uuid[index];
        // Separators are cosmetic -- "0000-1000-..." and "0000180F" must agree.
        if (c == '-' || c == ':' || c == ' ' || c == '\t') {
            continue;
        }
        const uint8_t nibble = hexNibble(c);
        if (nibble == 0xFF) {
            // Not hex. A malformed UUID must not be silently reinterpreted as a
            // short one -- that would print the wrong name next to real data.
            return false;
        }
        if (count >= sizeof(nibbles)) {
            return false;
        }
        nibbles[count++] = nibble;
    }

    if (count == 4) {
        out = nibblesToUint16(nibbles);
        return true;
    }
    if (count == 8) {
        // A 32-bit UUID only collapses to 16 bits when its high half is zero.
        if (nibbles[0] != 0 || nibbles[1] != 0 || nibbles[2] != 0 || nibbles[3] != 0) {
            return false;
        }
        out = nibblesToUint16(nibbles + 4);
        return true;
    }
    if (count == 32) {
        for (size_t index = 0; index < 24; ++index) {
            if (nibbles[8 + index] != kBaseUuidLowNibbles[index]) {
                return false;  // a genuine 128-bit vendor UUID
            }
        }
        if (nibbles[0] != 0 || nibbles[1] != 0 || nibbles[2] != 0 || nibbles[3] != 0) {
            return false;
        }
        out = nibblesToUint16(nibbles + 4);
        return true;
    }
    return false;
}

const char* GattNameTable::serviceName(uint16_t uuid) {
    const NamedUuid* entry = lookup(kServices, sizeof(kServices) / sizeof(kServices[0]), uuid);
    return entry != nullptr ? entry->name : "";
}

const char* GattNameTable::characteristicName(uint16_t uuid) {
    const NamedUuid* entry =
        lookup(kCharacteristics, sizeof(kCharacteristics) / sizeof(kCharacteristics[0]), uuid);
    return entry != nullptr ? entry->name : "";
}

const char* GattNameTable::descriptorName(uint16_t uuid) {
    const NamedUuid* entry =
        lookup(kDescriptors, sizeof(kDescriptors) / sizeof(kDescriptors[0]), uuid);
    return entry != nullptr ? entry->name : "";
}

bool GattNameTable::isVendorShortUuid(uint16_t uuid) {
    // 0xFF00..0xFFFF is where unregistered vendors squat in practice -- the LED
    // controllers and UART bridges this build targets all sit there. The
    // narrower 0xFFF0..0xFFFF block is what the spec reserves for duplicating
    // an assigned number without a separate license.
    return uuid >= 0xFF00;
}

std::string GattNameTable::resolve(const std::string& uuid, GattUuidKind kind) {
    uint16_t shortUuid = 0;
    if (!toShortUuid(uuid, shortUuid)) {
        return "";
    }
    const char* name = "";
    switch (kind) {
        case GattUuidKind::Service: name = serviceName(shortUuid); break;
        case GattUuidKind::Characteristic: name = characteristicName(shortUuid); break;
        case GattUuidKind::Descriptor: name = descriptorName(shortUuid); break;
    }
    return name;
}

const char* GattNameTable::kindLabel(GattUuidKind kind) {
    switch (kind) {
        case GattUuidKind::Service: return "service";
        case GattUuidKind::Characteristic: return "characteristic";
        case GattUuidKind::Descriptor: return "descriptor";
    }
    return "unknown";
}

uint16_t GattNameTable::serviceCount() {
    return static_cast<uint16_t>(sizeof(kServices) / sizeof(kServices[0]));
}

uint16_t GattNameTable::characteristicCount() {
    return static_cast<uint16_t>(sizeof(kCharacteristics) / sizeof(kCharacteristics[0]));
}

uint16_t GattNameTable::descriptorCount() {
    return static_cast<uint16_t>(sizeof(kDescriptors) / sizeof(kDescriptors[0]));
}

uint16_t GattNameTable::serviceUuidAt(uint16_t index) {
    if (index >= serviceCount()) {
        return 0;
    }
    return kServices[index].uuid;
}

const char* GattNameTable::serviceNameAt(uint16_t index) {
    if (index >= serviceCount()) {
        return "";
    }
    return kServices[index].name;
}

bool GattNameTable::tablesAreSorted() {
    return strictlyAscending(kServices, sizeof(kServices) / sizeof(kServices[0])) &&
           strictlyAscending(kCharacteristics, sizeof(kCharacteristics) / sizeof(kCharacteristics[0])) &&
           strictlyAscending(kDescriptors, sizeof(kDescriptors) / sizeof(kDescriptors[0]));
}
