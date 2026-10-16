# Third-party notices

Every library this firmware links against, what it is licensed under, and what
has to happen because of that. The audit is generated, not hand-maintained:

```
python build-reports/audit_licenses.py    # table + build-reports/license-audit.json
python build-reports/collect_licenses.py  # regenerates LICENSES/ from that audit
```

`audit_licenses.py` exits non-zero while any dependency blocks distribution, so
it doubles as a CI gate. **It exits non-zero today** — see *Blockers* below.

Full licence texts are in `LICENSES/`, copied byte for byte from the installed
library where one is shipped, and from the canonical SPDX text where the library
names a licence without shipping it.

---

## The table

| Library | Version | Licence | Class | In the `.bin`? |
|---|---|---|---|---|
| RF24 | 1.4.11 | **GPL-2.0-only** | strong copyleft | yes — **blocker** |
| arduinoFFT | 2.0.4 | **GPL-3.0** | strong copyleft | yes — **blocker** |
| ESP32 BLE Keyboard | 0.3.2 | **none found** | no grant | yes — **blocker** |
| ESP32 Arduino core | 3.20017.241212, 3.3.6 | LGPL-2.1-or-later | weak copyleft | yes |
| ESP-IDF prebuilt libs | 5.5.0 | LGPL-2.1-or-later | weak copyleft | yes |
| rc-switch | 2.6.4 | LGPL-2.1-or-later | weak copyleft | yes |
| TinyGPSPlus | 1.1.0 | LGPL-2.1-or-later | weak copyleft | yes |
| SmartRC-CC1101-Driver-Lib | 2.5.7 | bespoke permissive (`LicenseRef-…`, requires source reference) | permissive, conditional | yes |
| TFT_eSPI | 2.5.43 | MIT AND BSD-3-Clause | permissive | yes |
| Adafruit PN532 | 1.3.4 | BSD-3-Clause | permissive | yes |
| Adafruit BusIO | 1.17.4 | MIT | permissive | yes |
| ArduinoJson | 7.4.3 | MIT | permissive | yes |
| Unity | 2.6.1 | MIT | permissive | no — tests only |

Copyright lines belong to each library's own authors and are carried in
`LICENSES/` with the licence texts. They are not reproduced here, because a
hand-copied attribution drifts; the collected files do not.

---

## Blockers

Three findings stop this firmware from being distributable — and the first two are
*independent* reasons, so fixing one kind leaves the firmware just as undistributable as it
is now. That is why the gate keys on this script's exit code rather than on the copyleft
count alone. Details, evidence and the plan to close them:
`docs/legal/COMPLIANCE_PLAN_zh.md`, execution plan in
`docs/legal/LICENSE_REPLACEMENT_PLAN_zh.md`.

**1. RF24 (GPL-2.0-only) and arduinoFFT (GPL-3.0) cannot both be linked in.**
`RF24`'s `library.json` declares `GPL-2.0-only` — exactly v2, no "or later" — and
`arduinoFFT`'s LICENSE is the v3 text. Those two are mutually incompatible, so no
single licence covers the combined binary: it cannot be released under GPLv2, nor
under GPLv3, nor under this project's MIT. *The project's own licence cannot fix
this, because it only ever covered the project's own code.*

**2. ESP32 BLE Keyboard grants nothing.** No licence field, no LICENSE file, no
notice in the source headers, and GitHub reports no licence for the repository
either. Absent a grant, the default is all rights reserved; it is included by
`#include <BleKeyboard.h>` and instantiated, so it is compiled into the firmware,
and shipping that firmware redistributes it.

**3. The committed binaries are themselves the distribution.** Eleven `.bin`
files (18.8 MB) are tracked under `flash_package/` and `dist/`. `dist/` is in
`.gitignore`, but the files predate that entry and are still tracked. A public
repository distributes them, which means the two problems above are not
hypothetical. They also carry the upstream's trademark in their filenames.

---

## Weak copyleft: what is owed

The Arduino core cannot be replaced — it *is* the framework — so the LGPL-2.1
obligations have to be met rather than avoided. Static linking is permitted, but
the LGPL requires that the recipient be able to relink the work against a
modified version of the library. In practice that means shipping:

- the library sources, as used (unmodified: point at the upstream release);
- this project's sources, which are already public;
- **a means to relink** — the object files, or an equivalent the recipient can
  rebuild from.

`rc-switch` and `TinyGPSPlus` are in the same position, and both are small enough
that replacing them with in-house code is a real option (see the plan's track A).

---

## How each row was determined

The `Evidence` column of the audit output names the exact artifact each verdict
came from: a licence file, a source-file header, or a `library.json` /
`library.properties` / `package.json` declaration. Two classifications are worth
explaining, because a simpler rule gets them wrong:

- **GPL, not LGPL or AGPL.** The canonical GPLv2 text ends by suggesting "the GNU
  *Lesser* General Public License", and GPLv3's section 13 names the "GNU *Affero*
  General Public License". Scanning a file for those phrases reports RF24 as LGPL
  and arduinoFFT as AGPL. Both are plain GPL. The audit matches on the earliest
  occurrence, which lands on the title block.
- **TFT_eSPI is composite.** Its own terms are MIT, but its `LICENSE.txt` also
  carries the BSD text of the Adafruit_GFX functions it imported, so the row is an
  SPDX expression rather than a single id.
- **A bespoke notice can still be a grant.** SmartRC's `LICENSE.txt` says
  "You can freely use, edit or distribute it with reference to the source" —
  permissive, but conditional, and written in the author's own words instead of
  SPDX vocabulary. Reporting that as merely unidentified gets the conclusion
  wrong in the expensive direction: a reader concludes the grant is unknown when
  it is known and merely non-standard. It is recorded as a `LicenseRef` and its
  condition is printed as a NOTE — satisfied by this file and `NOTICE.md`, so it
  does not fail the gate, but it is a condition and must not be dropped when
  either file is edited.
- **`LGPL-2.1-or-later`, not bare `LGPL-2.1`.** `rc-switch` and `TinyGPSPlus`
  ship no licence file, and both headers say "either version 2.1 of the License,
  or (at your option) any later version". The bare GPL/LGPL ids in SPDX mean
  `-only`, so the earlier row had the direction of that clause backwards. The
  wording is now read for the version tag, and **only** source headers are read
  this way: every GPL/LGPL licence *text* contains "any later version" in its
  "How to Apply" appendix, so applying it to a licence file would wrongly
  upgrade RF24, whose `GPL-2.0-only` declaration has to win over the GPLv2 text
  it ships.
