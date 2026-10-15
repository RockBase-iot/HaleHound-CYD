#include "features/rfid/NdefCodec.h"

#include <cstdio>

namespace {

constexpr uint8_t kTnfWellKnown = 0x01;
constexpr uint8_t kTnfMediaType = 0x02;
constexpr uint8_t kTnfExternal = 0x04;

// NFC Forum URI RTD short-form codes 0x00-0x23.
const char* const kUriPrefixes[] = {
    "",                                  // 0x00
    "http://www.",                       // 0x01
    "https://www.",                      // 0x02
    "http://",                           // 0x03
    "https://",                          // 0x04
    "tel:",                              // 0x05
    "mailto:",                           // 0x06
    "ftp://anonymous:anonymous@",        // 0x07
    "ftp://ftp.",                        // 0x08
    "ftps://",                           // 0x09
    "sftp://",                           // 0x0A
    "smb://",                            // 0x0B
    "nfs://",                            // 0x0C
    "ftp://",                            // 0x0D
    "dav://",                            // 0x0E
    "news:",                             // 0x0F
    "telnet://",                         // 0x10
    "imap:",                             // 0x11
    "rtsp://",                           // 0x12
    "urn:",                              // 0x13
    "pop:",                              // 0x14
    "sip:",                              // 0x15
    "sips:",                             // 0x16
    "tftp:",                             // 0x17
    "btspp://",                          // 0x18
    "btl2cap://",                        // 0x19
    "btgoep://",                         // 0x1A
    "tcpobex://",                        // 0x1B
    "irdaobex://",                       // 0x1C
    "file://",                           // 0x1D
    "urn:epc:id:",                       // 0x1E
    "urn:epc:tag:",                      // 0x1F
    "urn:epc:pat:",                      // 0x20
    "urn:epc:raw:",                      // 0x21
    "urn:epc:",                          // 0x22
    "urn:nfc:",                          // 0x23
};

// Wi-Fi Simple Configuration attribute types used by a credential record.
constexpr uint16_t kWscCredential = 0x100E;
constexpr uint16_t kWscNetworkIndex = 0x1026;
constexpr uint16_t kWscSsid = 0x1045;
constexpr uint16_t kWscAuthType = 0x1003;
constexpr uint16_t kWscEncryptionType = 0x100F;
constexpr uint16_t kWscNetworkKey = 0x1027;

// Type-2 TLV tags.
constexpr uint8_t kTlvNdef = 0x03;
constexpr uint8_t kTlvTerminator = 0xFE;

void push16(std::vector<uint8_t>& out, uint16_t value) {
    out.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
    out.push_back(static_cast<uint8_t>(value & 0xFF));
}

void push32(std::vector<uint8_t>& out, uint32_t value) {
    out.push_back(static_cast<uint8_t>((value >> 24) & 0xFF));
    out.push_back(static_cast<uint8_t>((value >> 16) & 0xFF));
    out.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
    out.push_back(static_cast<uint8_t>(value & 0xFF));
}

uint16_t read16(const uint8_t* data) {
    return static_cast<uint16_t>((static_cast<uint16_t>(data[0]) << 8) | data[1]);
}

uint32_t read32(const uint8_t* data) {
    return (static_cast<uint32_t>(data[0]) << 24) | (static_cast<uint32_t>(data[1]) << 16) |
           (static_cast<uint32_t>(data[2]) << 8) | static_cast<uint32_t>(data[3]);
}

std::string hexByte(uint8_t value) {
    char buffer[4];
    std::snprintf(buffer, sizeof(buffer), "%02X", value);
    return std::string(buffer);
}

std::string formatFixed(double value, int decimals) {
    char buffer[40];
    std::snprintf(buffer, sizeof(buffer), "%.*f", decimals, value);
    return std::string(buffer);
}

uint8_t typeTnf(const NdefRecord& record) {
    switch (record.type) {
        case NdefRecordType::Url:
        case NdefRecordType::Text:
        case NdefRecordType::Geo:
        case NdefRecordType::Sms:
            return kTnfWellKnown;
        case NdefRecordType::VCard:
        case NdefRecordType::Wifi:
        case NdefRecordType::Bluetooth:
            return kTnfMediaType;
        case NdefRecordType::AndroidApp:
            return kTnfExternal;
        case NdefRecordType::Unknown:
        default:
            return kTnfWellKnown;
    }
}

NdefRecordType classify(uint8_t tnf, const std::string& typeName, const std::vector<uint8_t>& payload) {
    if (tnf == kTnfWellKnown && typeName == "U") {
        if (!payload.empty()) {
            const std::string uri = NdefCodec::expandUri(payload[0],
                                                         std::string(payload.begin() + 1, payload.end()));
            if (uri.compare(0, 4, "sms:") == 0) {
                return NdefRecordType::Sms;
            }
        }
        return NdefRecordType::Url;
    }
    if (tnf == kTnfWellKnown && typeName == "T") {
        return NdefRecordType::Text;
    }
    if (tnf == kTnfWellKnown && typeName == "geo") {
        return NdefRecordType::Geo;
    }
    if (tnf == kTnfMediaType && (typeName == "text/vcard" || typeName == "text/x-vcard")) {
        return NdefRecordType::VCard;
    }
    if (tnf == kTnfMediaType && typeName == "application/vnd.wfa.wsc") {
        return NdefRecordType::Wifi;
    }
    if (tnf == kTnfMediaType && typeName == "application/vnd.bluetooth.ep.oob") {
        return NdefRecordType::Bluetooth;
    }
    if (tnf == kTnfExternal && typeName == "android.com:pkg") {
        return NdefRecordType::AndroidApp;
    }
    return NdefRecordType::Unknown;
}

// Extracts the "FN" property from a vCard body so the viewer can show a name
// instead of the raw payload.
std::string vCardField(const std::string& body, const char* property) {
    const std::string needle = std::string(property) + ":";
    size_t cursor = 0;
    while (cursor < body.size()) {
        size_t lineEnd = body.find('\n', cursor);
        if (lineEnd == std::string::npos) {
            lineEnd = body.size();
        }
        std::string line = body.substr(cursor, lineEnd - cursor);
        while (!line.empty() && (line[line.size() - 1] == '\r' || line[line.size() - 1] == '\n')) {
            line.erase(line.size() - 1);
        }
        if (line.compare(0, needle.size(), needle) == 0) {
            return line.substr(needle.size());
        }
        cursor = lineEnd + 1;
    }
    return std::string();
}

// Reads the value of the first attribute with ``type`` from a WSC TLV stream.
bool wscAttribute(const std::vector<uint8_t>& payload,
                  uint16_t wanted,
                  std::vector<uint8_t>& value) {
    size_t cursor = 0;
    while (cursor + 4 <= payload.size()) {
        const uint16_t type = read16(&payload[cursor]);
        const uint16_t length = read16(&payload[cursor + 2]);
        cursor += 4;
        if (cursor + length > payload.size()) {
            return false;
        }
        if (type == wanted) {
            value.assign(payload.begin() + cursor, payload.begin() + cursor + length);
            return true;
        }
        cursor += length;
    }
    return false;
}

void appendAttribute(std::vector<uint8_t>& out,
                     uint16_t type,
                     const uint8_t* value,
                     uint16_t length) {
    push16(out, type);
    push16(out, length);
    out.insert(out.end(), value, value + length);
}

}  // namespace

const char* NdefCodec::uriPrefix(uint8_t code) {
    if (code >= uriPrefixCount()) {
        return "";
    }
    return kUriPrefixes[code];
}

uint8_t NdefCodec::uriPrefixCount() {
    return static_cast<uint8_t>(sizeof(kUriPrefixes) / sizeof(kUriPrefixes[0]));
}

uint8_t NdefCodec::abbreviateUri(const std::string& uri, std::string& rest) {
    uint8_t best = 0;
    size_t bestLength = 0;
    for (uint8_t code = 1; code < uriPrefixCount(); ++code) {
        const size_t length = std::string(kUriPrefixes[code]).size();
        if (length <= bestLength || uri.size() < length) {
            continue;
        }
        if (uri.compare(0, length, kUriPrefixes[code]) == 0) {
            best = code;
            bestLength = length;
        }
    }
    rest = best == 0 ? uri : uri.substr(bestLength);
    return best;
}

std::string NdefCodec::expandUri(uint8_t code, const std::string& rest) {
    return std::string(uriPrefix(code)) + rest;
}

NdefRecord NdefCodec::buildUrl(const std::string& url) {
    NdefRecord record;
    record.type = NdefRecordType::Url;
    record.typeName = "U";
    std::string rest;
    const uint8_t code = abbreviateUri(url, rest);
    record.payload.push_back(code);
    record.payload.insert(record.payload.end(), rest.begin(), rest.end());
    return record;
}

NdefRecord NdefCodec::buildText(const std::string& text, const std::string& language) {
    NdefRecord record;
    record.type = NdefRecordType::Text;
    record.typeName = "T";
    uint8_t languageLength = static_cast<uint8_t>(language.size());
    if (languageLength > 0x3F) {
        languageLength = 0x3F;
    }
    // Status byte: UTF-8 (bit 7 clear), language length in bits 5..0.
    record.payload.push_back(languageLength);
    record.payload.insert(record.payload.end(), language.begin(), language.begin() + languageLength);
    record.payload.insert(record.payload.end(), text.begin(), text.end());
    return record;
}

NdefRecord NdefCodec::buildVCard(const NdefVCard& card) {
    NdefRecord record;
    record.type = NdefRecordType::VCard;
    record.typeName = "text/vcard";
    std::string body = "BEGIN:VCARD\r\nVERSION:3.0\r\n";
    if (!card.formattedName.empty()) {
        body += "FN:" + card.formattedName + "\r\n";
    }
    if (!card.organization.empty()) {
        body += "ORG:" + card.organization + "\r\n";
    }
    if (!card.phone.empty()) {
        body += "TEL;TYPE=CELL:" + card.phone + "\r\n";
    }
    if (!card.email.empty()) {
        body += "EMAIL:" + card.email + "\r\n";
    }
    if (!card.url.empty()) {
        body += "URL:" + card.url + "\r\n";
    }
    body += "END:VCARD\r\n";
    record.payload.assign(body.begin(), body.end());
    return record;
}

NdefRecord NdefCodec::buildWifi(const NdefWifiCredential& credential) {
    NdefRecord record;
    record.type = NdefRecordType::Wifi;
    record.typeName = "application/vnd.wfa.wsc";

    std::vector<uint8_t> inner;
    const uint8_t networkIndex = 1;
    appendAttribute(inner, kWscNetworkIndex, &networkIndex, 1);

    const uint8_t* ssid = reinterpret_cast<const uint8_t*>(credential.ssid.data());
    appendAttribute(inner, kWscSsid, ssid, static_cast<uint16_t>(credential.ssid.size()));

    std::vector<uint8_t> auth;
    push16(auth, credential.authType);
    appendAttribute(inner, kWscAuthType, &auth[0], static_cast<uint16_t>(auth.size()));

    std::vector<uint8_t> encryption;
    push16(encryption, credential.encryptionType);
    appendAttribute(inner, kWscEncryptionType, &encryption[0], static_cast<uint16_t>(encryption.size()));

    const uint8_t* key = reinterpret_cast<const uint8_t*>(credential.password.data());
    appendAttribute(inner, kWscNetworkKey, key, static_cast<uint16_t>(credential.password.size()));

    push16(record.payload, kWscCredential);
    push16(record.payload, static_cast<uint16_t>(inner.size()));
    record.payload.insert(record.payload.end(), inner.begin(), inner.end());
    return record;
}

NdefRecord NdefCodec::buildBluetooth(const NdefBluetoothOob& oob) {
    NdefRecord record;
    record.type = NdefRecordType::Bluetooth;
    record.typeName = "application/vnd.bluetooth.ep.oob";

    uint8_t address[6] = {0, 0, 0, 0, 0, 0};
    {
        // Accept both "AA:BB:CC:DD:EE:FF" and a bare 12-digit hex string.
        std::string digits;
        for (size_t index = 0; index < oob.deviceAddress.size(); ++index) {
            const char character = oob.deviceAddress[index];
            if (character != ':' && character != '-') {
                digits.push_back(character);
            }
        }
        for (size_t slot = 0; slot < 6 && (slot * 2 + 1) < digits.size(); ++slot) {
            const std::string byteText = digits.substr(slot * 2, 2);
            address[slot] = static_cast<uint8_t>(std::strtoul(byteText.c_str(), NULL, 16) & 0xFF);
        }
    }

    std::vector<uint8_t> body;
    // Bluetooth OOB data begins with a 16-bit little-endian length, followed by
    // the device address stored least-significant byte first.
    const uint16_t bodyLength =
        static_cast<uint16_t>(6 + (oob.deviceName.empty() ? 0 : 2 + oob.deviceName.size()));
    push16(body, bodyLength);
    for (int index = 5; index >= 0; --index) {
        body.push_back(address[index]);
    }
    if (!oob.deviceName.empty()) {
        // EIR: length byte (type + value), type 0x09 = complete local name.
        body.push_back(static_cast<uint8_t>(oob.deviceName.size() + 1));
        body.push_back(0x09);
        body.insert(body.end(), oob.deviceName.begin(), oob.deviceName.end());
    }
    record.payload = body;
    return record;
}

NdefRecord NdefCodec::buildAndroidApp(const std::string& packageName) {
    NdefRecord record;
    record.type = NdefRecordType::AndroidApp;
    record.typeName = "android.com:pkg";
    record.payload.assign(packageName.begin(), packageName.end());
    return record;
}

NdefRecord NdefCodec::buildGeo(double latitude, double longitude) {
    NdefRecord record;
    record.type = NdefRecordType::Geo;
    record.typeName = "geo";
    const std::string text = formatFixed(latitude, 6) + "," + formatFixed(longitude, 6);
    record.payload.assign(text.begin(), text.end());
    return record;
}

NdefRecord NdefCodec::buildSms(const std::string& number, const std::string& body) {
    // No NFC Forum well-known type exists for SMS; the widely interoperable
    // representation is a URI record carrying the sms: scheme.
    std::string uri = "sms:" + number;
    if (!body.empty()) {
        uri += "?body=" + body;
    }
    NdefRecord record;
    record.type = NdefRecordType::Sms;
    record.typeName = "U";
    std::string rest;
    const uint8_t code = abbreviateUri(uri, rest);
    record.payload.push_back(code);
    record.payload.insert(record.payload.end(), rest.begin(), rest.end());
    return record;
}

NdefEncodeResult NdefCodec::encode(const NdefMessage& message) {
    NdefEncodeResult result;
    if (message.records.empty()) {
        result.detail = "empty message";
        return result;
    }

    for (size_t index = 0; index < message.records.size(); ++index) {
        const NdefRecord& record = message.records[index];
        const bool first = index == 0;
        const bool last = index + 1 == message.records.size();
        const bool shortRecord = record.payload.size() < 0x100;
        const uint8_t tnf = typeTnf(record);

        uint8_t header = 0;
        if (first) {
            header |= 0x80;  // MB
        }
        if (last) {
            header |= 0x40;  // ME
        }
        if (shortRecord) {
            header |= 0x10;  // SR
        }
        header |= static_cast<uint8_t>(tnf & 0x07);

        result.bytes.push_back(header);
        result.bytes.push_back(static_cast<uint8_t>(record.typeName.size()));
        if (shortRecord) {
            result.bytes.push_back(static_cast<uint8_t>(record.payload.size()));
        } else {
            push32(result.bytes, static_cast<uint32_t>(record.payload.size()));
        }
        result.bytes.insert(result.bytes.end(), record.typeName.begin(), record.typeName.end());
        result.bytes.insert(result.bytes.end(), record.payload.begin(), record.payload.end());
    }

    result.valid = true;
    result.detail = "encoded";
    return result;
}

NdefDecodeResult NdefCodec::decode(const std::vector<uint8_t>& bytes) {
    NdefDecodeResult result;
    size_t cursor = 0;
    bool sawMessageBegin = false;
    bool sawMessageEnd = false;

    while (cursor + 2 <= bytes.size()) {
        const uint8_t header = bytes[cursor++];
        if ((header & 0x80) != 0) {
            if (sawMessageBegin) {
                result.detail = "nested message begin";
                return result;
            }
            sawMessageBegin = true;
        }
        const bool shortRecord = (header & 0x10) != 0;
        const bool hasId = (header & 0x08) != 0;
        const uint8_t tnf = static_cast<uint8_t>(header & 0x07);

        const uint8_t typeLength = bytes[cursor++];
        uint32_t payloadLength = 0;
        if (shortRecord) {
            if (cursor + 1 > bytes.size()) {
                result.detail = "truncated length";
                return result;
            }
            payloadLength = bytes[cursor++];
        } else {
            if (cursor + 4 > bytes.size()) {
                result.detail = "truncated length";
                return result;
            }
            payloadLength = read32(&bytes[cursor]);
            cursor += 4;
        }

        uint32_t idLength = 0;
        if (hasId) {
            if (cursor + 1 > bytes.size()) {
                result.detail = "truncated id length";
                return result;
            }
            idLength = bytes[cursor++];
        }

        if (cursor + typeLength > bytes.size()) {
            result.detail = "truncated type";
            return result;
        }
        const std::string typeName(reinterpret_cast<const char*>(&bytes[cursor]), typeLength);
        cursor += typeLength;

        if (cursor + idLength > bytes.size()) {
            result.detail = "truncated id";
            return result;
        }
        cursor += idLength;

        if (cursor + payloadLength > bytes.size()) {
            result.detail = "truncated payload";
            return result;
        }
        NdefRecord record;
        record.payload.assign(bytes.begin() + cursor, bytes.begin() + cursor + payloadLength);
        cursor += payloadLength;
        record.typeName = typeName;
        record.type = classify(tnf, typeName, record.payload);
        result.message.records.push_back(record);

        if ((header & 0x40) != 0) {
            sawMessageEnd = true;
            break;
        }
    }

    if (!sawMessageBegin || result.message.records.empty()) {
        result.detail = "no records";
        return result;
    }
    result.valid = true;
    result.detail = sawMessageEnd ? "decoded" : "decoded without message end";
    return result;
}

NdefEncodeResult NdefCodec::encodeTlv(const NdefMessage& message) {
    NdefEncodeResult result = encode(message);
    if (!result.valid) {
        return result;
    }
    const size_t payloadSize = result.bytes.size();
    std::vector<uint8_t> tlv;
    tlv.push_back(kTlvNdef);
    if (payloadSize < 0xFF) {
        tlv.push_back(static_cast<uint8_t>(payloadSize));
    } else {
        tlv.push_back(0xFF);
        push16(tlv, static_cast<uint16_t>(payloadSize));
    }
    tlv.insert(tlv.end(), result.bytes.begin(), result.bytes.end());
    tlv.push_back(kTlvTerminator);
    result.bytes = tlv;
    result.detail = "encoded as type 2 tlv";
    return result;
}

NdefDecodeResult NdefCodec::decodeTlv(const std::vector<uint8_t>& bytes) {
    NdefDecodeResult result;
    size_t cursor = 0;

    // Type 2 tags start with a four-byte capability container (0xE1 version
    // size access). Skip it when present so the NDEF TLV is found in the right
    // place instead of assuming offset zero.
    if (bytes.size() >= 4 && bytes[0] == 0xE1 && (bytes[1] & 0xF0) == 0x40) {
        cursor = 4;
    }

    while (cursor < bytes.size()) {
        const uint8_t tag = bytes[cursor++];
        if (tag == kTlvTerminator) {
            break;
        }
        if (tag == 0x00) {
            continue;  // NULL TLV padding
        }
        if (cursor >= bytes.size()) {
            result.detail = "truncated tlv length";
            return result;
        }
        size_t length = bytes[cursor++];
        if (length == 0xFF) {
            if (cursor + 2 > bytes.size()) {
                result.detail = "truncated tlv length";
                return result;
            }
            length = read16(&bytes[cursor]);
            cursor += 2;
        }
        if (cursor + length > bytes.size()) {
            result.detail = "truncated tlv value";
            return result;
        }
        if (tag == kTlvNdef) {
            const std::vector<uint8_t> value(bytes.begin() + cursor, bytes.begin() + cursor + length);
            return decode(value);
        }
        cursor += length;
    }

    result.detail = "no ndef tlv";
    return result;
}

const char* NdefCodec::typeLabel(NdefRecordType type) {
    switch (type) {
        case NdefRecordType::Url:
            return "URL";
        case NdefRecordType::Text:
            return "Text";
        case NdefRecordType::VCard:
            return "Contact";
        case NdefRecordType::Wifi:
            return "WiFi";
        case NdefRecordType::Bluetooth:
            return "Bluetooth";
        case NdefRecordType::AndroidApp:
            return "App";
        case NdefRecordType::Geo:
            return "Location";
        case NdefRecordType::Sms:
            return "SMS";
        case NdefRecordType::Unknown:
        default:
            return "Unknown";
    }
}

std::string NdefCodec::summarize(const NdefRecord& record) {
    switch (record.type) {
        case NdefRecordType::Url:
            if (!record.payload.empty()) {
                return expandUri(record.payload[0],
                                 std::string(record.payload.begin() + 1, record.payload.end()));
            }
            return std::string();
        case NdefRecordType::Sms:
            if (!record.payload.empty()) {
                return expandUri(record.payload[0],
                                 std::string(record.payload.begin() + 1, record.payload.end()));
            }
            return std::string();
        case NdefRecordType::Text: {
            if (record.payload.empty()) {
                return std::string();
            }
            const uint8_t status = record.payload[0];
            const size_t languageLength = status & 0x3F;
            if (record.payload.size() <= 1 + languageLength) {
                return std::string();
            }
            return std::string(record.payload.begin() + 1 + languageLength, record.payload.end());
        }
        case NdefRecordType::VCard:
            return vCardField(std::string(record.payload.begin(), record.payload.end()), "FN");
        case NdefRecordType::Wifi: {
            std::vector<uint8_t> credential;
            if (wscAttribute(record.payload, kWscCredential, credential)) {
                std::vector<uint8_t> ssid;
                if (wscAttribute(credential, kWscSsid, ssid)) {
                    return std::string(ssid.begin(), ssid.end());
                }
            }
            return std::string();
        }
        case NdefRecordType::Bluetooth: {
            if (record.payload.size() >= 8) {
                std::string text;
                for (int index = 7; index >= 2; --index) {
                    text += hexByte(record.payload[index]);
                    if (index > 2) {
                        text += ":";
                    }
                }
                return text;
            }
            return std::string();
        }
        case NdefRecordType::AndroidApp:
        case NdefRecordType::Geo:
            return std::string(record.payload.begin(), record.payload.end());
        case NdefRecordType::Unknown:
        default:
            return hexByte(static_cast<uint8_t>(record.payload.size()));
    }
}

void ndefAppend(NdefMessage& message, const NdefRecord& record) {
    message.records.push_back(record);
}

void ndefClear(NdefMessage& message) {
    message.records.clear();
}
