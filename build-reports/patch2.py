import io, sys
def patch(path, edits):
    with io.open(path, 'r', encoding='utf-8', newline='') as fh:
        text = fh.read()
    for old, new, label in edits:
        n = text.count(old)
        if n != 1:
            sys.exit("PATCH FAIL %s [%s]: %d occurrences" % (path, label, n))
        text = text.replace(old, new, 1)
    with io.open(path, 'w', encoding='utf-8', newline='') as fh:
        fh.write(text)
    print("patched %s" % path)

patch('include/features/tuning/RadioTuningModel.h', [
    ("    uint8_t dataRateKbps = 2000;      // 250, 1000, 2000\n",
     "    uint16_t dataRateKbps = 2000;     // 250, 1000, 2000\n", "field"),
    ("    static bool acceptsDataRate(uint8_t kbps);\n",
     "    static bool acceptsDataRate(uint16_t kbps);\n", "signature"),
])
patch('src/features/tuning/RadioTuningModel.cpp', [
    ("bool RadioTuningModel::acceptsDataRate(uint8_t kbps) {",
     "bool RadioTuningModel::acceptsDataRate(uint16_t kbps) {", "impl"),
])
