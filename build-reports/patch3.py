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

# Parity semantics: a parity bit counted *with* its group gives the group's
# stated parity. An even-parity bit therefore equals ones(group) mod 2, i.e.
# oddParity(group); an odd-parity bit equals evenParity(group).
patch('src/features/rfid/IclassPacs.cpp', [
    ("""        // Leading parity is even, trailing parity is odd for these formats.
        result.leadingParityOk =
            leading == evenParity(raw, spec->leadingParityFrom, spec->leadingParityTo);
        result.trailingParityOk =
            trailing == oddParity(raw, spec->trailingParityFrom, spec->trailingParityTo);""",
     """        // The leading bit is an even-parity bit and the trailing bit an
        // odd-parity bit, each counted together with the group it covers. An
        // even-parity bit is therefore 1 when its group has an odd number of
        // ones, and an odd-parity bit is 1 when its group has an even count.
        result.leadingParityOk =
            leading == oddParity(raw, spec->leadingParityFrom, spec->leadingParityTo);
        result.trailingParityOk =
            trailing == evenParity(raw, spec->trailingParityFrom, spec->trailingParityTo);""",
     "parity"),
])

# The catalogue gained one nRF24 entry (NRF Tuning).
patch('test/native/test_catalog_lifecycle/test_main.cpp', [
    ("    TEST_ASSERT_EQUAL(6, countCategory(CategoryId::Nrf24));",
     "    TEST_ASSERT_EQUAL(7, countCategory(CategoryId::Nrf24));", "nrf count"),
])
