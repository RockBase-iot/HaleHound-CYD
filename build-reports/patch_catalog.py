import io, sys

def patch(path, edits):
    with io.open(path, 'r', encoding='utf-8', newline='') as fh:
        text = fh.read()
    original = text
    for old, new, label in edits:
        count = text.count(old)
        if count != 1:
            sys.exit("PATCH FAIL %s [%s]: expected 1 occurrence, found %d" % (path, label, count))
        text = text.replace(old, new, 1)
    if text == original:
        sys.exit("PATCH FAIL %s: no change" % path)
    with io.open(path, 'w', encoding='utf-8', newline='') as fh:
        fh.write(text)
    print("patched %s" % path)

patch('include/features/FeatureCatalog.h', [
    ("    BluetoothAirohaRace = 0x020A,\n",
     "    BluetoothAirohaRace = 0x020A,\n"
     "    BluetoothSkeletonKey = 0x020B,\n"
     "    BluetoothHidReader = 0x020C,\n"
     "    BluetoothKarr = 0x020D,\n",
     "bt ids"),
    ("    Nrf24ProtoKill = 0x0305,\n",
     "    Nrf24ProtoKill = 0x0305,\n"
     "    Nrf24Tuning = 0x0306,\n",
     "nrf id"),
    ("    TeslaCharge = 0x0406,\n",
     "    TeslaCharge = 0x0406,\n"
     "    SubGhzTuning = 0x0407,\n",
     "subghz id"),
    ("    NtagTools = 0x0505,\n",
     "    NtagTools = 0x0505,\n"
     "    RfidBackend = 0x0506,\n",
     "rfid id"),
])

patch('include/features/rfid/CardDumpFormat.h', [
    ("\nIFeature* createRfidBackendFeature();\n", "\n", "drop dup decl"),
])

patch('include/features/rfid/IclassPacs.h', [
    ("\nIFeature* createRfidBackendFeature();\n", "\n", "drop dup decl"),
])

patch('src/features/FeatureCatalog.cpp', [
    ('#include "features/rfid/NtagModel.h"\n',
     '#include "features/rfid/NtagModel.h"\n'
     '#include "features/rfid/RfidBackendModel.h"\n'
     '#include "features/ble/SkeletonKeyModel.h"\n'
     '#include "features/ble/HidReaderModel.h"\n'
     '#include "features/ble/KarrModel.h"\n'
     '#include "features/tuning/RadioTuningModel.h"\n',
     "includes"),
    ('    D_FACTORY(BluetoothAirohaRace, Bluetooth, "Airoha Race", Offensive, none, 0, none, 0, "ble", "/loot", createAirohaRaceFeature),\n',
     '    D_FACTORY(BluetoothAirohaRace, Bluetooth, "Airoha Race", Offensive, none, 0, none, 0, "ble", "/loot", createAirohaRaceFeature),\n'
     '    D_FACTORY(BluetoothKarr, Bluetooth, "KARR", Passive, none, 0, none, 0, "ble", "/loot/karr", createKarrFeature),\n'
     '    D_FACTORY(BluetoothSkeletonKey, Bluetooth, "SkeletonKey", SensitiveData, none, 0, none, 0, "ble", "/loot", createSkeletonKeyFeature),\n'
     '    D_FACTORY(BluetoothHidReader, Bluetooth, "HID Reader", Passive, none, 0, none, 0, "ble", "/loot", createHidReaderFeature),\n',
     "bt descriptors"),
    ('    DL(Nrf24ProtoKill, Nrf24, "Proto Kill", Offensive, {PeripheralId::Nrf24}, 1, none, 0, "spi", "/captures", createLegacyNrf24ProtoKill),\n',
     '    DL(Nrf24ProtoKill, Nrf24, "Proto Kill", Offensive, {PeripheralId::Nrf24}, 1, none, 0, "spi", "/captures", createLegacyNrf24ProtoKill),\n'
     '    D_FACTORY(Nrf24Tuning, Nrf24, "NRF Tuning", Passive, {PeripheralId::Nrf24}, 1, none, 0, "spi", "/settings", createNrf24TuningFeature),\n',
     "nrf descriptor"),
    ('    D_FACTORY(TeslaCharge, SubGhz, "Tesla Charge", Offensive, {PeripheralId::Cc1101}, 1, none, 0, "spi", "/subghz", createTeslaChargeFeature),\n',
     '    D_FACTORY(TeslaCharge, SubGhz, "Tesla Charge", Offensive, {PeripheralId::Cc1101}, 1, none, 0, "spi", "/subghz", createTeslaChargeFeature),\n'
     '    D_FACTORY(SubGhzTuning, SubGhz, "SubGHz Tuning", Passive, {PeripheralId::Cc1101}, 1, none, 0, "spi", "/settings", createSubGhzTuningFeature),\n',
     "subghz descriptor"),
    ('    D_FACTORY(NtagTools, Rfid, "NTAG Tools", SensitiveData, {PeripheralId::Pn532}, 1, none, 0, "i2c", "/loot", createNtagToolsFeature),\n',
     '    D_FACTORY(NtagTools, Rfid, "NTAG Tools", SensitiveData, {PeripheralId::Pn532}, 1, none, 0, "i2c", "/loot", createNtagToolsFeature),\n'
     '    D_FACTORY(RfidBackend, Rfid, "RFID Reader", Passive, none, 0, {PeripheralId::Pn532}, 1, "select", "/settings", createRfidBackendFeature),\n',
     "rfid descriptor"),
])
print("OK")
