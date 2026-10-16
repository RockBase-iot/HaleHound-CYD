# Notice: derivation, attribution and trademarks

This file states what this firmware is derived from, what was changed, what
attribution it carries, and what it must not be called. It complements `LICENSE`
(the licence for this project's own code) and `THIRD_PARTY_NOTICES.md` (the
libraries it links).

---

## 1. What this is

A firmware for ESP32 "Cheap Yellow Display" boards, providing a wireless security
toolkit: 802.11 monitoring and testing, BLE tooling, 2.4 GHz (nRF24) and Sub-GHz
(CC1101) radio functions, RFID, GPS logging and a serial console. It runs on the
`esp32-cyd` and `nm-cyd-c5` targets.

It is a **derivative work**, not an original codebase.

---

## 2. Provenance

| | |
|---|---|
| Upstream | **HaleHound-CYD** — https://github.com/JesseCHale/HaleHound-CYD |
| Upstream author | **Jesse C. Hale** (JesseCHale) |
| Upstream's own origin | **ESP32-DIV** by **CiferTech** (MIT) |
| Forked from | upstream commit **`68bbfe6`**, 2026-03-16 |
| Licence at that point | **MIT** |
| Modified by | **RockBase-iot**, from commit **`c9d7598`**, 2026-07-27 onward |

This repository carries the upstream's own commit history: 93 of its 116 commits
are the upstream author's, dated 2026-02-17 to 2026-03-16. That history is the
evidence that the code was obtained under MIT, and **it must not be rewritten** —
squashing or rebasing it would destroy the record.

Two facts matter for anyone auditing this:

1. **The upstream was still MIT when this fork was taken.** It replaced its
   licence with a proprietary text later, somewhere between its `v3.5.0` and
   `v3.6.2` releases. A licence already granted is not withdrawn by that change.
2. **The upstream later rewrote its history.** Its current `main` and this
   repository share no common ancestor (`git merge-base` between them is empty),
   and twelve of its tags all point at the same commit. Upstream changes cannot be
   merged from here, and none of this project's code comes from a version
   published under the proprietary terms.

The upstream's MIT grant, exactly as received, is preserved verbatim in
[`LICENSES/HaleHound-CYD-MIT.txt`](LICENSES/HaleHound-CYD-MIT.txt). It is a copy,
not a summary: the original 324-line file included the upstream's branding, and
it is kept whole precisely so that the record of the grant is unambiguous.

---

## 3. Modifications

The upstream's MIT-era documentation asked forks to *clearly indicate the
modifications made*. This is that statement, at a level of detail that can be
checked against the history:

- **Build system** — converted from an Arduino sketch build to PlatformIO, with
  two board environments (`esp32-cyd`, `nm-cyd-c5`) and a set of build-time
  feature flags.
- **Structure** — added `include/` and `src/` layers for a feature runtime
  (`core/`), hardware transport seams (`drivers/`), the feature catalogue and its
  menu model (`features/`), services, board and platform definitions — alongside
  the original flat translation units, which are kept so the upstream code stays
  diffable.
- **Board support** — added an `nm-cyd-c5` target (ESP32-C5, ST7789 panel, 16 MB
  partition table, PSRAM) that does not correspond to any upstream board.
- **Features** — added a PN532 RFID stack (transport / protocol / framing /
  session), a BLE central seam with protocol clients, an authorization policy
  gate on the feature runtime, and a menu built from the feature catalogue.
- **Testing** — added a host-side test suite (`test/native/`) that runs the
  pure-logic layers, plus the tooling in `build-reports/`.

Nothing was added from any upstream release published after the fork point.

---

## 4. Attribution to carry

Wherever this firmware is distributed — an About screen, a README, a product
page, packaging, a release note — carry at least:

> Based on HaleHound-CYD by Jesse C. Hale (MIT), which in turn derives from
> ESP32-DIV by CiferTech (MIT).

And, wherever it could be read as claiming endorsement, also:

> Not affiliated with, endorsed by, or sponsored by the HaleHound project,
> Jesse C. Hale, or CiferTech.

Both lines are short on purpose. The first is an obligation: MIT conditions
redistribution on retaining the copyright and permission notices. The second is a
statement of fact, and it is what keeps the attribution above from being read as
a claim of association.

---

## 5. Trademarks

**MIT grants no trademark rights.** It does not mention trademarks at all, and
silence is not permission. "HaleHound" is used by the upstream as a brand for
goods of exactly this kind — firmware for exactly this hardware, sold alongside
it — so using it for this project's own product is a confusion risk, not a
theoretical one.

What is allowed is **nominative use**: the name may appear to identify what this
work came from, which is what the attribution above does.

What is not allowed:

- using `HaleHound` as this project's name, product name, icon, or asset;
- naming the firmware or its build artifacts after it;
- letting a device broadcast or display it as its own identity;
- implying a relationship, approval, or common origin beyond the attribution.

See `docs/legal/COMPLIANCE_PLAN_zh.md` §3 for the specific sites in the tree that
currently violate this, including one where the device announces itself to nearby
phones under the upstream's name.

---

## 6. Responsible use

Several functions in this firmware disrupt other people's radio traffic, and
several collect it. Run them only on equipment you own or have written
authorisation to test, and only where local radio regulation permits.

Jamming and unauthorised interception are criminal offences in most
jurisdictions, including under 47 U.S.C. § 333 in the United States. Radio
transmission may require a licence. The authors accept no liability for misuse.

---

## 7. Current compliance status

**This tree is not yet a distributable release.** Three dependency findings
block it — a GPL-2.0-only library alongside a GPL-3.0 one (mutually
incompatible, so no licence can cover the combined binary), a library that grants
no licence at all, and eleven committed `.bin` files that are themselves the act
of distribution. Read
[`THIRD_PARTY_NOTICES.md`](THIRD_PARTY_NOTICES.md) for the list and
[`docs/legal/COMPLIANCE_PLAN_zh.md`](docs/legal/COMPLIANCE_PLAN_zh.md) for the
evidence and the plan.

This section exists so that the state of the tree is not mistaken for a shipping
state.

---

## 8. Verifying any of the above

```bash
# The upstream's commits are in this history, authored by the upstream author.
git log --format='%h %an %ad %s' --date=short | tail -25

# The fork point, and its date.
git log -1 --format='%h %ad %s' --date=short 68bbfe6

# The licence as it was at the fork point.
git show 68bbfe6:LICENSE | grep -c 'MIT LICENSE'

# The upstream's current licence is proprietary, and its history is disjoint.
git fetch https://github.com/JesseCHale/HaleHound-CYD.git main
git show FETCH_HEAD:LICENSE | head -20
git merge-base HEAD FETCH_HEAD   # prints nothing: no common ancestor

# The dependency audit, which exits non-zero while blockers remain.
python build-reports/audit_licenses.py

# Every licence text this project must ship with.
python build-reports/collect_licenses.py
```
