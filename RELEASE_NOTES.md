# CANPAN3 / CANDISP firmware — release notes

Module: CANPAN3 (and CANDISP / CANSCAN builds from the same source) on PIC18F27Q83.
Version reported to MMC/FCU is `5a<build>` where `<build>` is `PARAM_BUILD_VERSION` in `module.h`.

Baseline for these notes: **upstream 5a13** (spikyian/CANPAN3 commit fa5f96b, 4 Aug 2026), the
most recent build in the upstream repository. Every entry below describes a difference from
that baseline.

Convention from build 33 onwards: every change bumps `PARAM_BUILD_VERSION` and gets an entry
here. Code changes are tagged `// KeithB bNN:` in the source so they can be found with grep.

**Submitted upstream:** everything up to and including 5a36 (CANPAN3 application changes and the
`KeithB b14-25`/`b35`/`b36` library patches) was sent to Ian Hogg (spikyian) on 20 Sep 2026.
Entries from 5a37 onwards are changes made on top of that submission; the 5a36 state is the
reference point when comparing against whatever upstream publishes next.

---

## 5a58 — 7 Oct 2026 — KeithB — switches closed at power-up no longer send events

- The b35 switch debounce (5a35) treated the first full scan after power-up like any other:
  `rawState` starts at 0, so a switch already closed at power-up was ignored on the first scan
  and acted on at the second, once `canpanScanReady` was set, sending its event. The first full
  scan now takes the rows as read, as before 5a35, so those switches set their state without
  sending events. Same change as Ian merged in his CANPAN3 `keithb` (PR #2, 1cc9cab). Tagged
  `KeithB b58`.
- Ian has also merged PR #1 (state saving) and PR #11 (build output not tracked); this tree
  already had both.
- **Library, keithb-v13:** the Q83 configuration template has `WDTE = SWDTEN` again (Ian's
  template had `OFF`). The unified hex build failed with hexmate "conflicts with existing data
  at address 0x300004" (CONFIG5) because the bootloader's `hwsettings.c` has `SWDTEN`; the two
  must be identical. No change in behaviour: the watchdog stays off unless software sets
  `WDTCON0.SEN`, which CANPAN3 never does. The K80 template also differs from the bootloader
  (`BORPWR`, `BBSIZ`); not changed here, as no K80 unified build is made. Raised with Ian.

To test: power up with some switches closed; no events should be sent for them, and toggling
them afterwards should behave normally.

## 5a57 — 6 Oct 2026 — KeithB — in line with Ian's keithb library

Built against the `keithb-v13` branch of github.com/KeithMerg/VLCBlib_PIC: Ian's `keithb` at
4b2c3eb, which has taken follow-up patches 0001 (asyncEEPROM build fix, f4a0669) and 0002
(`VLCB_VDD_WRITE_GUARD`, 4b2c3eb) from `github\VLCBlib_PIC-keithb-60e677e-followups.zip`, plus
two commits Ian does not have yet:

- 0003, which fixes the `EVENT_TABLE_HEAL_ERASED` loop (`NUM_EVENTS`, renamed
  `PARAM_NUM_EVENTS`); sent to Ian by email. Without it this build does not compile.
- `__reentrant` on every `processMessage()`, the workaround for the XC8 compiled-stack overlap
  (Microchip case 01901775) that made NVRD/NVSET fail, carried over from keithb-v12 (5a56).
  Ian's `keithb` does not have it, so without it the fault could come back. The changes below follow the way Ian's CANPAN3 `keithb` (1b83d7f)
handles the new library; the CANPAN3 PRs to Ian are unaffected.

- **EEPROM.** Switch and LED states now go through the library's buffered writer
  (`ASYNC_EEPROM BUFFER` in `module.h`): `readNVM()`/`writeNVM()` replace `readEEvalue()`/
  `writeEEvalue()`, and `EEPROMbuffer.c`/`.h` are out of the project (`asyncEEPROM_buffer.c` in).
  The library version has the 5a14–5a25 fix (write, verify on the next poll, clear the flag only
  if it matches) and is polled every main-loop pass, so `loop()` no longer calls the writer.
  Bytes outside the buffer (NVs, NN, CANID, boot flag) are written directly as before. The
  factory-reset clearing in `main.c`, which wrote straight to EEPROM behind the old buffer's
  back, now goes through the buffer, so its RAM copy can no longer go stale.
- **Configuration words.** `main.c` no longer includes `vlcb_config_q83.h`/`_k80.h`: Ian's
  `vlcb.c` includes the template itself, and a second copy would set the words twice. His
  templates differ from the bootloader's `hwsettings.c` in `WDTE` (OFF, was SWDTEN) on the Q83,
  and `BORPWR` (MEDIUM) and `BBSIZ` (BB2K) on the K80. CANPAN3 does not use the watchdog.
- **`TICK_ONCE_PER_PASS` removed.** The library now always reads the tick once per pass.
- **`APP_earlyInit()`** added (empty): the library calls it first thing in `main()`.
- **`VLCB_VDD_GUARD 0x0B`** defined, as in Ian's CANPAN3: power-up waits for Vdd above the HLVD
  level (typ. 4.00 V) instead of a fixed ~1 s. `VLCB_VDD_WRITE_GUARD` (refuse NVM writes while
  Vdd is low, keithb 4b2c3eb) is in `module.h` commented out.
- `canpan3Nv.c` includes `ticktime.h` (for `HALF_SECOND`), as upstream.

Checked with gcc against stubbed XC8 headers for CANPAN3, CANDISP and CANSCAN; not yet built
with XC8.

To test: switch and LED states restored after a power cycle (with the startup NV set to restore them);
power-up time (should be ~50 ms on a good supply); the build's configuration words; plus the
5a49–5a56 items.

## 5a56 — 5 Oct 2026 — KeithB — library moved to the keithb-v12 branch

No application code change; `PARAM_BUILD_VERSION` only, so a module reports which library it
was built with.

- **Library.** `VLCBlib_PIC` is now the `keithb-v12` branch of github.com/KeithMerg/VLCBlib_PIC
  (f120502, the pull request Ian is taking from), replacing keithb 89356f9 plus the v10 patch
  set. That branch is Ian's `keithb` at 41eb28b (his forms of patches 01, 02, 04, 05 and 07–12)
  plus patches 03, 06 and 13–22, the config templates made identical to the bootloader's
  `hwsettings.c` (WRTB/WRTC on), and `__reentrant` on every `processMessage()` as the
  workaround for the XC8 compiled-stack overlap (Microchip case 01901775), which showed as
  NVRD/NVSET failing. Both laptops track the branch with GitHub Desktop.
- **`module.h`** (edited 30 Sep): `CAN_ENUM_BEFORE_FIRST_TX` is now `CAN_ADDITIONAL_CANID_CHECKS`,
  Ian's name for the option (keithb 7224ef1); without the rename the fix would have silently
  switched off. `CAN_RESERVE_MSG_RAM` removed: Ian took the reservation out (5c12aa2). It was
  never needed: the linker is given the CAN RAM banks as SFR space and cannot place variables
  there (checked in the 5a55 map file), so 5a39 changed nothing in the hex.
- **Withdrawn, Ian was right:** the 5a37 `getNumRxBuffersInUse()` change. For a receive FIFO
  `C1FIFOUA3` is the read pointer and `FIFOCI` the index of the next slot to be written, so the
  original arithmetic was correct; patch 03 no longer touches it. The RX buffer usage and high
  watermark diagnostics therefore report the exact count again.
- Still on offer to Ian, not on either branch: the CAN service / hardware split (project doc
  `VLCBlib-can-service-split-proposal.md`).

To test (nothing since 5a49 has been on the bench): the 5a49–5a55 items, plus NVRD and NVSET
from MMC repeated a few dozen times, which is what the compiled-stack overlap broke.

## 5a55 — 28 Sep 2026 — KeithB — configuration words moved out of the library

- Following Ian's view that the library should set no configuration words, `vlcb.c` no longer
  contains any `#pragma config`. The two sets are now templates, `vlcb_config_q83.h` and
  `vlcb_config_k80.h`, with exactly the values of CBUS_PIC_Bootloader `hwsettings.c` (checked
  pragma by pragma: 69 on the Q83, 37 on the K80). Library tag `KeithB b55`; patch 15 of the
  series offered to Ian (`github\VLCBlib_PIC-keithb-patches-v10`), which replaces the earlier
  "config words match the bootloader" patch and `VLCB_APP_CONFIG`.
- CANPAN3 `main.c` includes the template for its family, so its hex carries the same words
  as before 5a55 (the Q83 values have matched the bootloader since 5a41).

To test: the build's configuration words (MPLAB X Window > Target Memory Views >
Configuration Bits, or the hex) are the same as 5a54's.

## 5a54 — 28 Sep 2026 — KeithB — erased event rows cleared at power-up

- `EVENT_TABLE_HEAL_ERASED` (`module.h`, defined): at power-up the event teach service scans
  the event table once and clears any row whose flags byte and EN are all 0xFF. Such a row
  can only come from a power cut between a flash page erase and its write (or from a table
  area that was never written); it would otherwise read as an event with EN 0xFFFF, show in
  NERD, count as used and never be taught over. The library only writes 0 or 1 to the flags
  byte, so no taught event can match. Nothing is written unless such a row is found. Library
  tag `KeithB b54`; patch 23 of the series offered to Ian
  (`github\VLCBlib_PIC-keithb-patches-v9`); patch 22 from v10 on.
- Not repaired: a row straddling a page boundary that lost only its NN or EVs.
- Checked against the data sheet while looking at this: `PIR0bits.CANIF` is read-only
  (DS40002265C, PIR0) and clears itself when the CAN module's own flags are cleared, which
  the CAN error handler does. Nothing to fix there.

To test: normal start-up unchanged (no extra flash writes); after an MMC restore, events
read back as before.

## 5a53 — 28 Sep 2026 — KeithB — enumeration retries slowly on a bus with no free CANID

- `can18_can_2.c` (`CAN_ENUM_BEFORE_FIRST_TX`): after an enumeration that found no free CANID
  (5a52), the next one now waits `ENUMERATION_RETRY` (default 5 s, can be set in `module.h`)
  instead of the 200 ms hold-off, and queuing another frame no longer starts one early; the
  frame is held and goes out after the retry. Before this, a module on a full bus sent an
  enumeration request about every 300 ms and again with every frame it queued. Tagged
  `KeithB b53`; part of patch 02 of the series offered to Ian
  (`github\VLCBlib_PIC-keithb-patches-v8`).
- No change on a normal bus.

## 5a52 — 28 Sep 2026 — KeithB — no frame sent as CANID 0 when no CANID is free

- `can18_can_2.c` (`CAN_ENUM_BEFORE_FIRST_TX`, defined in CANPAN3): if the self-enumeration
  finds no free CANID (every one from 1 to 99 taken), the frames held in the TX FIFO stay
  held and the enumeration is repeated after the 200 ms hold-off. Previously they were
  released carrying CANID 0. A repeated enumeration starts with the held frames still
  marked as waiting, so they go out with the CANID it finds. Tagged `KeithB b52`; part of
  patch 02 of the series offered to Ian (`github\VLCBlib_PIC-keithb-patches-v7`).
- No change on a normal bus: this path is only taken when the bus has no free CANID.

## 5a51 — 28 Sep 2026 — KeithB — EVLRNI and NENRD withdrawn from the simple event teach service

- Ian's view (23 Sep) is that EVLRNI and NENRD belong to the indexed event teach service
  (`event_teach_indexed.c`), not the simple one. The two `case`s added in 5a35 are removed
  from the library's `event_teach_simple.c`, and the patch is dropped from the series offered
  to Ian, which is now 22 patches (`github\VLCBlib_PIC-keithb-patches-v6`). The library is
  again exactly Ian's `keithb` (89356f9) plus that series.
- No effect on CANPAN3 in use: it offers the simple (old CBUS) event teach service, which MMC
  and FCU teach with EVLRN and read with NERD and REVAL, as upstream CANPAN3 always has.
  `OPC_EVLRNI` stays in the learn-mode list in `canpan3Events.c` `APP_preProcessMessage()`,
  as upstream has it.

To test: MMC teach, read back and restore of events as before.

## 5a50 — 28 Sep 2026 — KeithB — event flash page-write change (5a48) withdrawn

- The 5a48 change (`FLASH_DEFER_EVENT_FLUSH`, library tag `KeithB b48`) is removed from
  both CANPAN3 and the library, and dropped from the series offered to Ian, which is now
  23 patches (`github\VLCBlib_PIC-keithb-patches-v5`). The library is again exactly
  Ian's `keithb` (89356f9) plus that series.
- Effect: the event flash page is written after every EVLRN again, as upstream. The LED
  matrix pulses during an MMC restore as it did before 5a48 (each page erase/write stops
  the CPU and the LED interrupt for a few ms). An MMC backup is not affected (5a26–5a31 fix).
- Nothing else changes: without the deferral no event page is ever left pending, so the
  flushes 5a48 added before NNRST, NNRSM and BOOT are not needed.

To test: MMC restore works and the events read back correctly (pulsing during the
restore is expected); teach an event and power-cycle straight away — it is kept.

## 5a49 — 28 Sep 2026 — KeithB — library brought into line with Ian's keithb branch

Where Ian's `keithb` branch of VLCBlib_PIC already fixes something that this fork also
fixed, Ian's version is now used, so the local library is exactly `keithb` (89356f9) plus the
24-patch series offered to him (`github\VLCBlib_PIC-keithb-patches-v2`).

- Interrupt priorities: Ian enables them always (`IPEN = 1`, library ISRs low priority);
  the local opt-in `VLCB_IPEN` (b43) is gone. **Application change:** the TMR2 LED-matrix
  handler in `canpan3Outputs.c` is now declared `low_priority` to match `TMR2IP = 0`.
  With every interrupt at low priority, none pre-empts another, as before.
- `canWaitForTxQueueToDrain()`: Ian's version (returns a `TxDrainResult`, gives up after
  `TX_DRAIN_TIMEOUT_MS`, default 500 ms) replaces the local 200 ms one. The K80 driver gains
  the same function.
- AREQ/ASRQ: answered only when `APP_isProducedEvent()` says so (Ian's form; CANPAN3 already
  provides it) instead of the `EVENT_UNKNOWN` test.
- Node number: Ian's eebdc67 clears the saved NN going from Normal to Uninitialised; the
  local "save the NN whenever it changes" (`last_nn`, b36) is replaced by that plus one
  small addition for the case eebdc67 does not cover (cancelling Setup restores the NN and
  now saves it too).
- CAN priority table: Ian's entries, with OPC 0xFC (DDWS) at low priority rather than normal.
- Two small additions that are in the patch series but were not in the local tree:
  `BUS_OFF_ERROR` in `TxDrainResult`, and a 0x3F fallback for the CAN BRP when
  `CAN_CLOCK_MHz` is not defined (CANPAN3 defines it, so no change here).

To test: normal operation, status LEDs, LED matrix brightness and flash (the TMR2 handler
now saves context as a low-priority interrupt); NNRSM with the module alone on the bus
should still reset within about half a second; put the module in Setup with MMC, cancel
with a short button press, power-cycle and check it keeps its NN.

## 5a48 — 28 Sep 2026 — KeithB — LEDs no longer pulse during an MMC restore

First made as 5a47 on 26 Sep 2026. That copy was overwritten when both source folders on
kbr7560 were replaced on 27 Sep, so it is re-applied here as build 48; `b47` is now taken
by the library's LCR options (see below).

- **Fix: LED matrix pulsed while MMC restored the module** (backup was fixed in 5a26–5a31).
  A restore sends one EVLRN per EV, and `addEvent()` wrote the event flash page after every
  one: a page erase and a page write, during each of which the Q83 CPU is stopped and
  interrupts are held off. The 100 µs LED interrupt could not run, so the row that happened to
  be lit stayed on and the others went dark, several hundred times over a restore. No change
  to the LED driver can cover this; the only cure is fewer flash writes.
- With `FLASH_DEFER_EVENT_FLUSH` (`module.h`, defined) `addEvent()` leaves the page in the
  RAM page buffer, which already serves all event reads, so the hash table rebuild, REVAL,
  NERD etc. see the new data at once. The page is written when teaching moves on to another
  page (as before), once flash writes have been quiet for 1 s, at most 5 s after the last
  flush while writes continue, and before NNRST, NNRSM and BOOT. A restore now stalls the CPU
  about once per 256-byte page (about every 14 events on CANPAN3) instead of once or twice
  per EV; it also saves flash wear.
- Consistency: only one page can differ from flash, and each flush brings flash fully up to
  date, so after a power cut the event table is exactly as it stood at the last flush.
- Trade-off: an event taught less than a second before power is removed is lost. Comment
  out `FLASH_DEFER_EVENT_FLUSH` for the old write-per-EVLRN behaviour.
- Library patches tagged `KeithB b48`: `nvm.c`/`nvm.h` (`flashWriteActivity` flag set by
  `FLASH_Write()`), `vlcb.c` (`poll()` quiet-period flush), `event_teach_simple.c`
  (`addEvent()` flush made conditional), `mns.c` and `boot.c` (`flushFlashBlock()` before
  `RESET()`, unconditional and harmless when nothing is pending). Offered upstream as patch
  0024 of the keithb series.

**Library builds with no entry of their own here** (none is defined in CANPAN3's
`module.h`, so they do not change this module): `b44` `CANID_PREFERRED`, `b45`
`VLCB_RX_PER_POLL`, `b46` `VLCB_EARLY_INIT` / `VLCB_VDD_GUARD`, `b47` the LCR-001..007 options
(`VLCBlib_PIC/CHANGES_b47.md`).

To test: MMC restore of a populated module with several LEDs lit — the matrix should hold
steady, with at most a brief flicker per flash page. Then power-cycle a few seconds after
the restore finishes and compare a fresh backup with the file restored; teach one event,
power-cycle within half a second, and confirm that event (only) is missing.

## 5a43 — 22 Sep 2026 — KeithB — port A set-up per hardware (CANSCAN button fix)

- `setup()` set `WPUA = 0b00001000` and made RA5 an output driven low on every build. That
  came from upstream 5a13, which only knows the CANPAN3 board, and was merged unconditionally in
  5a34. On **CANSCAN** RA5 is the push button: once `setup()` ran it read "pressed" for good
  and was never seen released, so the button did nothing (no setup mode, no NN change). The
  power-on button check runs before `setup()` and was not affected. On **CANDISP** the RA2
  button lost its weak pull-up. Now the pull-ups match each board's `APP_setPortDirections()`
  and RA5 is driven only on CANPAN3. Tagged `KeithB b43`.
- RA4 (unused on all three boards; pin 6 is VDDCORE/VCAP on the PIC18F25K80) is still driven
  low, latch first, then direction.
- Bootloader (`hwsettings.h`, HW_CANPAN3 configuration, which also serves CANSCAN and CANDISP):
  RA4 driven low from the bootloader onwards; RA5 left alone. See BOOTLOADER.md item 14.

## 5a42 — 21 Sep 2026 — KeithB — fixes from an independent review of 5a41 (three of them b40 regressions)

An independent read-through of every file compiled into 5a41, plus the BL4 bootloader, with the
new (b35–b41) code scrutinised hardest. Findings, all fixed here, tagged `KeithB b42` / `BL5`.

**Regressions introduced in b40 — 5a40 and 5a41 should not be flashed**

- `module.h` included `statusLeds.h` (which pulls in `ticktime.h`) *before* `#define
  TICK_ONCE_PER_PASS`, so `ticktime.h` never saw the define in most translation units:
  `tickNow` was declared but never refreshed, and `statusLeds2.c` compared against a value stuck
  at zero. On hardware the status LEDs would have stopped flashing (Setup yellow permanently off,
  first received message leaving the green LED on for ever), and the one-tick-per-pass saving
  was not delivered. The define now sits above the include, and the main loop writes
  `tickNow.val = tickGet()` explicitly rather than through the macro.
- `mns.c`: the b40 rewrite of the MODE handler dropped the `return NOT_PROCESSED` after the
  inner switch, so a MODE SETUP/UNINITIALISED request to a module not in Normal mode fell
  through into `OPC_NNRST` and reset it — a global `MODE 00 00 00` would have reset every
  uninitialised module on the bus. Return restored.
- `vlcb.c`: the b40 `PB_TIMEOUT` early return in `checkPowerOnPb()` skipped the b36
  release-wait, so a power-on press held past 28 s was again seen by `mnsPoll()` as a >4 s
  press and dropped the module to Uninitialised. Timeout now falls through to the wait.

**Other fixes**

- `can18_can_2.c` (`CAN_ENUM_BEFORE_FIRST_TX`): a frame queued while an enumeration hold-off
  was pending (`ENUMERATION_REQUIRED`, e.g. two fresh CANID-0 modules seeing each other's RTR)
  was never re-stamped or released. That state now starts the enumeration at once with the
  frame marked waiting.
- `event_teach_simple.c`: the `findEvent()` cache is also keyed on the module's own NN, since
  rows taught with `EVENT_FLAG_DEFAULT` resolve to `nn.word` and a NN change (button to
  Uninitialised, Setup-cancel) was not invalidating it.
- `event_consumer_simple.c`: the b40 "reuse the `isConsumedEvent()` result" edit had not
  actually landed (two row reads per consumed event); now it has.
- `event_producer_simple.c`: `producerEsdData()` answered indices 0/1 where ESD asks 1..3
  (same slip as the b38 consumer fix).
- `canpan3Outputs.c`: `rowBright0/1` and the `LED_FAST_DIM` walking pointers are plain RAM
  pointers rather than `const uint8_t *`, so XC8 cannot decide to route them through its
  mixed-space accessor inside the 100 µs ISR.

**Bootloader BL_VERSION 5 (`KeithB BL5`)**

- `bl_romops.c` `BLOCK()`: `(int)h << 8` sign-extended for `h >= 0x80` on the 16-bit `int`,
  so pages `0x00_8xxx` and `0x01_8xxx` compared equal; unsigned 24-bit arithmetic now.
- `main.c`: the memory type is classified from the frame's own address, not the
  auto-incremented one — a data frame ending at `0x1FFFF` pushed `u` to 2 and was dropped.

Reviewed and fine (for the record): `priorities[]` 256 entries verified; `findEvent()` cache
invalidated on every NN/EN write path; `evs[]` never read after a different row's `getEVs()`;
`ledBright[]`/`rowBright` indexing for CANPAN3 and CANDISP; `LED_FAST_DIM` bit-identical to the
old loop; brightness cache reloaded after factory reset; ISR/mainline sharing; NVM unlock
sequences; bootloader `sendReadReply()`/`sendByteReply()` object layout, `bufferDirty` flush
points, verify loop, and the SID = 0 filter/mask values.

Bootloader is at BL5 and needs rebuilding (`CBUS_PIC_Bootloader.X`) before `CANPAN3.X`.

## 5a41 — 21 Sep 2026 — KeithB — bootloader BL_VERSION 4, matching boot-block size

**Application:** `vlcb.c` config `BBSIZE` changed from `BBSIZE_512` to `BBSIZE_1024` so the
hardware-protected boot block covers the whole bootloader (0x000–0x7FF) and agrees with the
bootloader's own config image in the unified hex. No code change; `KeithB b41`.

**Bootloader (CBUS_PIC_Bootloader, BL_VERSION 3 → 4, tagged `KeithB BL4`)** — all of the review
findings in `BOOTLOADER.md` section 9:

- Self-verify now works: `MODE_SELF_VERIFY` defined in `main.h` (the old `SELF_VERIFY` define
  matched nothing), and the Q83 verify reads each written page back with `TBLRD` and compares it
  with the page buffer. A failed write now gives NAK on the next ACK and NOK at `CHK_RUN`.
- Flash and config read-back load `TBLPTR` as well as `NVMADR`, so `PG` reads return the
  requested address instead of wherever `TBLPTR` happened to be.
- Config-write loop advances `NVMADR` (which `writeConfigByte()` uses), not `TBLPTR`.
- Receive filter matches extended frames with SID = 0 (the PC's ID) as the K80 build does, so
  another module's boot replies (SID 0x400) are no longer taken as commands.
- `BBSIZE_1024` in `hwsettings.c` (with the application change above).
- `writeFlashByte()` waits for the READPAGE to finish before restoring `NVMADR` and writing the
  buffer; the page buffer is only flushed when it holds unwritten data (`bufferDirty`), on a
  page change and at `CHK_RUN` / `RESET` — an erase/write pair per control frame is gone.
- Read loops send 8 bytes (`<`), not 9 (`<=`).
- `C1CONT` written with REQOP = Configuration during set-up; the PLL is waited for (`ORDY`)
  before CAN is started; `WPUA` no longer enables the RA5 pull-up.

Size: the first BL4 build overflowed 0x800 by about 80 bytes on kbr7560 (XC8 v3.10 at -O2; the
0x75E figure for BL3 came from kbr7510 with XC8 v4.00 at -O3, which generates smaller code).
The three 8-byte read replies and four 1-byte replies, each written out in full, are now two
helpers (`sendReadReply()`, `sendByteReply()`), and flash/config reads use `TBLRD*+` instead of
a hand-rolled 3-byte increment. Result: 0x788 of 0x800 (1928 bytes, 94 %) on XC8 v3.10 -O2. The bootloader only
changes when flashed with a programmer (unified hex); modules updated over CAN keep BL_VERSION 3,
which is fine — BL3 and the 5a41 application are compatible either way, only the boot-block
config word differs and that is written by the programmer, not over CAN.

To test after a programmer flash: a normal MMC/FCU firmware load should behave exactly as before
(and ESD for the boot service should report version 4); a deliberate write to a bootloader
address should NAK; two modules in boot mode at once should no longer disturb each other.

## 5a40 — 21 Sep 2026 — KeithB — performance review fixes, remaining spec items

Implements the XC8 performance review (project doc `CANPAN3-xc8-performance-review.md`,
findings verified against the 5a39 listing) apart from item K, and clears the cosmetic spec items
left from 5a38. Everything is tagged `KeithB b40`. Two of the larger changes are switchable in
`module.h`: `LED_FAST_DIM` and `TICK_ONCE_PER_PASS` (both defined).

**LED driver (`canpan3Outputs.c`, review A / H / I)**

- The per-step dimming loop in the 100 µs TMR2 interrupt used a variable shift (`1U << i`,
  a rotate loop on the PIC18) and rebuilt the array index every iteration: ~20 µs of every
  100 µs period. Rewritten with a rolling mask and a walking pointer (`LED_FAST_DIM`); expected
  ~5 µs. The original loop is kept under `#else` for A/B comparison.
- The brightness NVs are cached in RAM (`ledBright[]`, loaded in `initOutputs()` and kept
  current by `updateLedBrightness()` from `APP_nvValueChanged()`), so the ISR no longer makes
  8 (16 on CANDISP) `getNV()` calls at every row change, and `getNV()` is no longer reachable
  from an interrupt — the duplicate copy XC8 had to carry (`i2_getNV`) should drop out of the image.
- `setLed()` / `clearLed()` / `testLed()` use an 8-entry mask table instead of `1 << (no%8)`.

**Main loop timing (review B / C / J)**

- `tickGet()` (35 instructions plus a TMR0 interrupt disable/enable) was called about seven
  times per idle pass. With `TICK_ONCE_PER_PASS` the main loop in `vlcb.c` refreshes a shared
  `tickNow` once and `poll()`, `mnsPoll()`, `leds_poll()` and the application `loop()` compare
  against it (`tickTimeSinceNow()` / `tickNowGet()` in `ticktime.h`). The busy-wait button timers
  in `vlcb.c` refresh it themselves so the status LEDs keep flashing during the power-on sequence.
- `leds_poll()`: the six `flashCounter/25`, `/50`, `/100` expressions, whose result was only ever
  used as true/false, are now `>=` comparisons; the `___lbdiv` library routine is no longer needed.
- The timed-response period is precomputed (`timedResponseTicks`) instead of a widening
  multiply on every `poll()`.

**Event handling (review D / E / F / G)**

- `event_teach_simple.c`: the 24-bit row address (`EVENT_ROW_ADDRESS()`) is computed once per
  access in `getEv()`, `getEVs()`, `getNN()`, `getEN()` and `clearTableEntry()` instead of once
  per byte; `getEVs()` walks a single incrementing address.
- `findEvent()` keeps a one-entry cache of the last successful lookup. An incoming event was
  looked up twice with the same NN/EN (application pre-processing, then the consumer service),
  each a hash-chain walk of up to 20 rows. Invalidated by `rebuildHashtable()`,
  `clearTableEntry()` and `addEvent()`.
- `canpan3Events.c`: `APP_isConsumedEvent()` reads the row once with `getEVs()` instead of up
  to nine `getEv()` calls; `APP_processConsumedEvent()` walks the flag/polarity bytes with a
  rolling mask and skips any byte that cannot affect an LED (same decisions as before, including
  the flash-turns-others-off rule).
- `event_consumer_simple.c`: `isConsumedEvent()` result reused instead of being recomputed
  for the EVENTACK check.
- `can18_can_2.c`: the priority table lookups are done once per frame, not twice.

**Spec items left from 5a38**

- Unknown MODE values get `GRSP_INVALID_MODE` (mns); `MODE_EVENT_ACK_ON/OFF` reply `GRSP_OK`
  from the consumer service.
- `RDGN` error GRSPs use `SERVICE_ID_MNS` rather than a literal 1; REVAL with a bad event index
  reports `CMDERR_INV_EN_IDX`.
- Power-on button sequence: `pbUpTimer()`/`pbDownTimer()` return `PB_TIMEOUT` (0xFF) on a
  timeout, so releasing and re-pressing within a second no longer aborts the factory-reset
  sequence. A power-on press shorter than one second still does nothing.

**Not done, deliberately**

- Review item K (`#pragma optimize 1` at the top of `nvm.c` de-optimises the whole file): the fix
  needs the NVM unlock sequences checked in the `.lst` before and after, on a real build.
  Offered as a follow-up once 5a40 has been built and the listing is available.
- Build-setting experiments (`optimization-invariant-enable`, `wpo-lto`, favour speed): project
  settings, not source; each wants its own build and listing diff.
- `mnsPoll()`'s `__reentrant` qualifier: added upstream (DL7BJ) for XC8 3.x, left alone.

Testing notes: LED brightness NVs changed over MMC must take effect immediately (cache path);
LED matrix should look identical with `LED_FAST_DIM` on and off; Setup-mode yellow flash and the
power-on reset-warning flash must still run (tick refresh in the busy loops); events with LEDs
in several flag bytes, and the flash-turns-others-off 4-aspect case, should behave as before.

## 5a39 — 20 Sep 2026 — KeithB — CAN message RAM reserved from the linker

- `can18_can_2.c`: the 0x450 (1104) bytes of CAN message RAM at 0x3BB0–0x3FFF used by the
  TXQ and the three FIFOs are now declared as `static volatile uint8_t canMsgRam[] __at(0x3BB0)`
  and `C1FIFOBA` is loaded from the array's address, so the linker knows the region is in use and
  cannot place anything else there. The size is derived from the FIFO definitions, so it tracks
  any change to the queue sizes. Selected by `CAN_RESERVE_MSG_RAM` in `module.h` (defined);
  comment it out if the linker refuses an absolute address above its data space (the build
  summary shows data space ending at 0x3200, so the address may be reported as outside RAM).
- Note: 0x3800 with 1536 bytes would only cover 0x3800–0x3DFF and miss the top of the FIFO
  area; the array is placed at the driver's own base address for that reason.

## 5a38 — 20 Sep 2026 — KeithB — remaining upstream-only items, tidy-up

Clears most of the "still upstream-only" list. Library patches tagged `KeithB b38`.

**CAN driver (`can18_can_2.c`)**

- Self-enumeration reply now carries the current CANID. FIFO1 holds a single preloaded reply
  frame, so it is always full when the CANID changes and `prepareSelfEnumResponse()` could never
  refill it; the queued frame's ID byte is now patched in place.
- A module with CANID 0 (fresh from factory reset) now enumerates *before* its first
  transmission and sends the queued frame with the CANID it obtains. Upstream forced CANID 1 and
  transmitted at once, marking enumeration as "required" but leaving the frame already on the bus.
  Selected by `CAN_ENUM_BEFORE_FIRST_TX` in `module.h` (defined); comment it out to restore the
  upstream behaviour. Frames queued while the enumeration is running are held and released with it.
- `canWaitForTxQueueToDrain()` gives up after 200 ms. Alone on the bus a frame is never
  acknowledged and TXREQ never clears, so NNRSM sat in this loop instead of resetting.
- `C1CONT` is written with REQOP = Configuration (0x54) rather than Normal FD (0x50) during
  set-up, so the mode change is only requested once bit timing, FIFOs and filters are in place.
  Previously it worked because the mode change waits for bus idle.
- `TXIE` no longer enabled: no TX FIFO interrupt sources are configured and nothing cleared TXIF.
- Unused `canTransmitFailed`, the stray global `EnumerationState`, and unused locals removed.

**Other library**

- `vlcb.c`: `priorities[]` had five entries missing (0x0B, 0x0E, 0x0F, 0x62, 0xFC), so every
  opcode above 0x0A was reading the priority of a later opcode, up to five places out. For this
  module the practical effect was PNN, DGN and ESD going out at NORMAL rather than LOW priority
  and SQU losing HIGH. Table now has 256 entries, verified against the opcode labels.
- `nv.c`: GRSP replies for NVRD/NVSET/NVSETRD carry `SERVICE_ID_NV` (the responding service)
  instead of `SERVICE_ID_MNS`.
- `event_consumer_simple.c`: `consumerEsdData()` answered index 0, but ESD data is requested with
  indices 1–3, so ESD byte 1 was always 0. Now returns `CONSUMER_EV_NOT_SPECIFIED` for index 1.

**Application**

- `canpan3Outputs.c`: `pollOutputsOld()` (the pre-interrupt LED driver, never called) removed.

Still upstream-only after this build: CAN message RAM (0x3BB0–0x3FFF) is not declared to the
linker (addressed in 5a39).
Cosmetic items left alone: unknown MODE values and EVENT_ACK on/off get no GRSP; a failed NVSET
sends CMDERR only (the GRSP is commented out in `nv.c`); the power-on factory-reset sequence
aborts if the button is re-pressed within 1 s of release.

To test on hardware: factory reset, then press the button to enter Setup — MMC should see the
RQNN and the module should acquire a free CANID (RDGN CAN service diag 13/15 count the
enumeration and CANID change); NNRSM with the module alone on the bus should now reset within a
second rather than hanging.

## 5a37 — 20 Sep 2026 — KeithB — CAN driver tidy-ups, second laptop

Sources copied to the second laptop (kbr7510) at 5a36; MPLAB project there
(`MPLABXProjects\CANPAN3.X`) references `..\..\github\{CANPAN3,VLCBlib_PIC,VLCB-defs}` as before.
`VLCB-defs\vlcbdefs_enums.h` verified identical to upstream HEAD (the "one commit behind" note in
"Libraries" below applies to the git metadata only).

Library patches tagged `KeithB b37`, all in `can18_can_2.c` (first changes to the CAN driver):

- `getNumRxBuffersInUse()`: for a receive FIFO both `C1FIFOUA3` and `FIFOCI` describe the read
  side, so the old arithmetic always returned 0 — the RX_BUFFER_USAGE and RX high-watermark
  diagnostics never showed anything. The peripheral has no readable write pointer, so the count is
  now estimated from the FIFO status flags: 0 / 1 (not empty) / 16 (half full) / 32 (full).
  Coarse, but it will show FIFO pressure during an MMC backup where before it showed nothing.
- `canSendMessage()`: an event is queued for self-consumption (COE) only after the TX-FIFO full
  check passes. Previously a failed send still acted on the event locally while nothing went out
  on the bus.
- `processEnumeration()`: the scan for a free CANID tested the array element before the array
  bound (one byte over-read when all 127 IDs are taken). Bound tested first.
- `processEnumeration()`: frames re-stamped with a new CANID after a TX-waiting enumeration keep
  their priority bit (byte 0 was overwritten whole). Currently unreachable because of the known
  CANID-0 issue, but correct if that is ever fixed.

Reviewed and left alone (spec-compliance only, no functional effect): `nv.c` GRSP/CMDERR replies
use `SERVICE_ID_MNS` rather than `SERVICE_ID_NV` and a failed NVSET sends no GRSP;
`consumerEsdData()` has a `case 0` that is never asked for (ESD byte 1 is always 0); unknown
MODE values and EVENT_ACK on/off get no GRSP; the power-on factory-reset sequence aborts if the
button is re-pressed within 1 s of release (`pbUpTimer()` returns whole seconds). One item worth
a datasheet check: `canPowerUp()` writes `C1CONT = 0x50` (REQOP = Normal FD) before the bit-timing
and FIFO registers are configured; it works because the mode change waits for bus idle, but it is
relying on timing.

Still upstream-only (unchanged from 5a36): CAN priority table misaligned; self-enumeration reply
slot keeps the old CANID; CANID-0 node transmits as CANID 1 without enumerating; `TXIE` enabled
with no handler; `canWaitForTxQueueToDrain()` has no timeout; NNRSM can hang if alone on the
bus; CAN message RAM / NVM buffer not reserved from the linker.

## 5a36 — 20 Sep 2026 — KeithB — remaining review fixes (library, not the CAN driver)

Library patches tagged `KeithB b36`. The CAN driver (`can18_can_2.c`) and the CAN priority
table in `vlcb.c` are deliberately left as upstream for now.

- `event_teach_simple.c`
  - REVAL with EV#0 (non-FCU mode): the NEVAL stream now carries the module's NN instead of
    the event's, so configuration tools accept it.
  - Teaching an event whose hash chain is already full (20 events in one bucket) is refused
    with `CMDERR_TOO_MANY_EVENTS` instead of being stored where it could never be found.
  - `errno` cleared at the start of `addEvent()`; a stale error no longer makes a later
    successful EVLRN report failure.
  - `getEv()` returns a negative error for an invalid index (was positive, read as a value).
  - NNCLR / factory reset clear all rows first and flush + rebuild the hash table once, instead
    of per row (was O(n²): ~2–3 s of blocking on blank flash, enough to overrun the RX FIFO).
- `mns.c`
  - The node number is persisted whenever it changes. Cancelling SETUP restored it in RAM only
    (module rebooted Normal with NN 0); going Uninitialised via the button cleared it in RAM
    only (module rebooted still answering its old NN).
  - `last_mode_state` initialised at power-up, so the mode byte is no longer rewritten to
    EEPROM on every boot; mode byte written with `MODE_NVM_TYPE`.
  - SETUP-mode short/long press now requires the press to have been seen (`pbWasPushed`),
    as the Normal-mode path already did.
- `vlcb.c`: after the power-on button sequence, wait for the button to be released and
  restart the MNS button timer. A press held past the 28 s timeout was being seen by
  `mnsPoll()` as a >4 s press and dropped the module to Uninitialised.
- `event_producer_simple.c`: AREQ/ASRQ answered only for events this module produces; a
  consume-only event replied AROF/ARSOF and contradicted the real producer.
- `nvm.c`
  - The flash page buffer is tracked with a `loaded` flag. It was initialised as if page
    0x800 (the parameter block) were loaded, so reads of 0x800–0x8FF returned uninitialised
    RAM until the first flash write (the boot service reads that range at power-up).
  - `EEPROM_Write()` gives up after three failed verifies instead of retrying forever.
  - Duplicate erase in `FLASH_Write()` removed (`flushFlashBlock()` already erases).

Still upstream-only (not patched): CAN priority table misaligned; self-enumeration reply slot
keeps the old CANID; CANID-0 node transmits as CANID 1 without enumerating; `TXIE` enabled
with no handler; `canWaitForTxQueueToDrain()` has no timeout; NNRSM can hang if alone on the
bus; CAN message RAM / NVM buffer not reserved from the linker.

## 5a35 — 20 Sep 2026 — KeithB — code review fixes, switch debounce

Result of a full review of the application and the library files compiled into it.

**Application**

- Switch inputs are debounced: a change must be seen on two consecutive scans of a column
  (16 ms apart) before it is acted on. Removes double toggles / spurious ON-OFF pairs from
  contact bounce, at the cost of 16 ms extra latency. (`canpan3Inputs.c`)
- When a switch number is re-assigned to a different event, the superseded event is removed
  through the library (`removeEvent()`) so the event hash table stays valid. Previously its EN
  bytes were zeroed directly in flash and `findEvent()` could still return the dead row.
- Factory reset now clears the saved LED states (EE 32–63) as well as the switch states.
- `canpanSetAllSwitchOff()` removed (never called).

**Library (VLCBlib_PIC, local patches tagged `KeithB b35`; to be offered upstream)**

- `mns.c`: `RDGN` for a service without diagnostics (COE, boot) fell through to a NULL
  function pointer — any node could reset the module with `RDGN <NN> 7 1`. Now replies and stops.
- `event_consumer_simple.c`: short events (ASON/ASOF) were looked up with the sender's NN
  instead of 0, so a short event taught in the normal way was never consumed.
- `event_teach_simple.c`: `EVLRNI` and `NENRD` were not handled (silently dropped); both
  implemented. `NENRD` reply now returns the requested index rather than the internal table index.
- `boot.c`: `OPC_BOOT` now checks the message length before comparing the NN bytes.
- `nv.c`: `NVSET` of NV#0 is rejected (NV#0 holds the NV layout version and is read-only;
  `NVRD` of it answers with the NV count).
- `vlcb.c`: the power-on-button factory reset is followed by a `RESET()` so the NV cache and
  service state reload the defaults instead of staying stale until the next power cycle.

Further library findings that were **not** patched here (upstream candidates, see the review
notes): CAN priority table misaligned by 1–5 entries; self-enumeration reply slot keeps the old
CANID; CANID-0 node transmits as CANID 1 without enumerating; REVAL/NEVAL stream uses the event's
NN; hash-chain overflow silent; NN not written to EEPROM on SETUP-cancel / go-Uninitialised;
`canWaitForTxQueueToDrain()` and `EEPROM_Write` have no timeout; NNCLR is O(n²).

## 5a34 — 20 Sep 2026 — KeithB — in step with upstream 5a13

- Merged upstream 5a13: RA5 driven low as an unused output and its weak pull-up dropped
  (WPUA 0b00101000 → 0b00001000 in `module.h`, CANPAN3 section). The fork now carries every
  upstream change up to 5a13.
- Every difference from upstream 5a13 in the CANPAN3 sources is now tagged in the code with
  `// KeithB bNN:` (`b14-25` where the exact build is not known). No functional change.
- Library folders checked against their GitHub upstreams (see "Libraries" below).

## 5a33 — 20 Sep 2026 — KeithB — bug fixes and LED driver optimisations

**Fixes**

- Switch states are now restored at power-up when NV1 (startup) says so. `initInputs()` was
  testing the NV *index* rather than its value, so restore never happened (regression versus
  upstream, introduced in the 14–25 series). Note: modules that ran builds 14–32 have been *saving*
  switch states all along, so the first power-up on 5a33 will restore whatever is in EEPROM; a
  factory reset clears it if the states look wrong.
- Paired switches (NV switch-mode bit 0) now save their state in the even switch's EEPROM slot,
  which is where `outputState` keeps it. Previously the odd slot was written, so a pair turned
  OFF came back ON after a power cycle. (Also present in upstream 5a13.)
- `saveSwitchState()` addresses EEPROM via `EE_ADDR_SWITCHES` rather than the raw index.
- `addTestEvent()` (test mode) no longer shifts by a negative count for switches below 9/17/25 —
  undefined behaviour that happened to work with the current compiler.
- Internal weak pull-ups removed from the four switch-row inputs on CANPAN3; the board has
  external pull-downs (RN1) and the rows go high when a button is pressed.
- `EEPROMbuffer.c`: `#import` → `#include`; bounds checks on `writeEEvalue()`/`readEEvalue()`.

**Optimisations**

- LED matrix dimming step: the row's 8 (16 on CANDISP) brightness NVs are copied into RAM once
  per row change instead of calling `getNV()` on every 100 µs step; the new cathode pattern is
  computed *before* the TLC5917 outputs are blanked, and an SPI transfer is only made when the
  pattern actually changes. Most steps now send nothing, so the outputs are blanked far less
  and the interrupt is shorter.
- Flash-rate check in `loop()` uses a precomputed period in ticks instead of a 32-bit divide on
  every iteration.
- CANDISP: `latchCathodes()` sets the SPI transfer count to 2 so the SS/LE line spans both
  cascaded TLC5917 bytes (untested on CANDISP hardware at the time of writing).

## 5a32 — 20 Sep 2026 — KeithB — LED ghosting fixed

- Dim glow on LEDs that should be off (present since the BUSY-wait change in the 14–25 series;
  upstream 5a12/5a13 do not have it). Root cause: the TLC5917 output latch is transparent while LE is high,
  and LE is driven by the SPI SS line, so during a byte transfer the LED outputs follow the
  shift register bit by bit. The `SPI1CON2bits.BUSY` wait clears before the last bits have
  clocked out, so OE was re-enabled while the lit bit was still rippling past the other outputs
  (~250 ns flash on every other LED in the row, 16 times per row ≈ 0.25 % duty). Restored the
  upstream `SPI1STATUSbits.TXBE` wait, which only sets once the byte is fully out.
  Selected by `LED_SPI_WAIT_TXBE` in `module.h` (defined).
- `LED_ROW_BLANK_US` and `LED_GHOST_SUPPRESS` (added in 5a27–5a31 while chasing the above)
  left in as options but switched off: they address charge storage on the floating row/column
  lines, which turned out not to be the visible problem.

## 5a26 – 5a31 — 19–20 Sep 2026 — KeithB — LED matrix driven from a timer interrupt

- **Fix: LEDs dimmed/pulsed while MMC was backing the module up.** `pollOutputs()` (one
  software-PWM step of the LED matrix) was called from the main loop, so each step's length —
  and therefore the LED duty cycle and refresh rate — depended on how long the loop iteration
  took. Every VLCB message handled during a backup stretched a step. `pollOutputs()` is now
  called from a TMR2 interrupt every 100 µs (`LED_MATRIX_ISR`, `LED_MATRIX_ISR_PERIOD_US` in
  `module.h`; comment out `LED_MATRIX_ISR` to revert to the main-loop call).
- Removed a duplicated `brightness += 2` on the row-change step, which gave each row 15 PWM
  steps instead of 16 and skipped brightness level 2.
- `APP_writeLED1/2` (status LEDs on LATB6/7) rewritten so the compiler emits single BSF/BCF
  instructions; a multi-instruction read-modify-write of LATB could otherwise race the LED
  interrupt, which drives the row anodes on LATB4/5.
- LED matrix options gathered into one block in `module.h` inside
  `#if HARDWARE==HW_CANPAN3 || HARDWARE==HW_CANDISP`.
- Diagnostic option `LED_DIAG_NO_DIMMING` (off) writes each row once and skips the per-step
  dimming transfers; used to bisect the ghosting.

## 5a14 – 5a25 — May–Sep 2026 — KeithB — multi-hardware support and local changes

This fork branched from upstream 5a12 (6c32145, 22 May 2026). Cumulative differences from
upstream 5a13, reconstructed from the source diff:

- One source tree now builds CANPAN3, CANDISP (64-LED, two cascaded TLC5917s, 21 EVs/event) and
  CANSCAN (128 switches, no LEDs) selected by the `HARDWARE` define; all pin assignments moved
  to per-hardware macros in `module.h`.
- **Event table moved from 0x1E800 to 0x1E000** in flash so that CANDISP's larger event rows
  fit. A module upgraded from 5a12/5a13 will not see its old events — do an MMC restore or
  re-teach after the first flash.
- Flash-ON events can now turn *other* LEDs off (for 4-aspect signals): with LED mode = Flash,
  an ON event turns off any LED whose "active" flag is clear and "invert" flag is set.
  (Contributed upstream by Keith Bruce as 6c32145.)
- `doFlash()` only runs while at least one LED is in a flashing state (`doFlashEnabled`), and
  the flash-rate NV is cached rather than read every loop.
- EEPROM writer (`EEPROMbuffer.c`): writes are verified on the next poll and retried if they
  did not take (fixes occasional LED/switch state not restored after power-up); an
  `atLeastOneWriteNeeded` flag avoids scanning the buffer when idle; the poll is throttled to
  once per ms (upstream re-armed the timer but never updated it).
- Bootloader service moved to the end of the service list; service index numbers reported by
  RQSD/ESD therefore differ from upstream (cosmetic).
- `startupNv` cached in the inputs and LEDs modules.
- Various array sizes guarded with `N ? N : 1` so the CANSCAN build (no LEDs) and CANDISP
  build (no buttons) compile.

## Libraries (checked 20 Sep 2026)

- **VLCBlib_PIC** (github.com/spikyian/VLCBlib_PIC): local copy is at upstream HEAD
  (f441600, 24 May 2026, "Fix for RQSD MNS service index") plus the local patches tagged
  `KeithB b14-25` (`nvm.h` declares `EEPROM_Read()` for `EEPROMbuffer.c`), `b35`, `b36`, `b37`, `b38`, `b39`, `b40`, `b41`, `b42` and `b43`–`b47`, on top of Ian's `keithb` branch (89356f9) since 5a49 (`b48` withdrawn in 5a50).
  No upstream commits since; one PR ever merged upstream (DL7BJ, `__reentrant` for XC8 3.x).
- **VLCB-defs** (github.com/Versatile-LCB/VLCB-defs): `vlcbdefs_enums.h` is identical to upstream
  HEAD (76534cb). The git checkout is one commit behind (a687e3b, `CANID_TRAINTASTIC` 0x7A);
  no effect on this module.
- **CBUS_PIC_Bootloader**: reviewed, see "Bootloader" below. **cbusdefs**: not checked.

## Bootloader (CBUS_PIC_Bootloader, reviewed 20 Sep 2026 — fixed as BL_VERSION 4 in 5a41)

Local copy dated May 2026 (BL_VERSION 3, PIC18F27Q83 build 0x75E of 0x800 bytes). Findings, in
order of importance; none affect a normal FCU/MMC firmware load, which only writes and checksums:

- Program-flash and CONFIG *reads* (PG bit set) on the Q83 return the wrong data: the data-frame
  handler loads the address into NVMADR only, but `readFlashByte()` is a `TBLRD*` using TBLPTR,
  which is never loaded (it starts at 0 and just advances). Fix: load TBLPTRU/H/L alongside
  NVMADR in the data-frame handler.
- CONFIG *writes* on the Q83 advance TBLPTR between bytes while `writeConfigByte()` uses NVMADR,
  so every byte of a frame goes to the same address. Masked in practice because the application
  config sets `WRTC = ON` (config write-protected). Fix: increment NVMADRL/H instead.
- The receive filter accepts *every* extended-ID frame (filter and mask only test EXIDE). The
  K80 version also matched SID = 0, the PC-to-module ID; the Q83 version therefore also accepts
  other modules' bootloader replies (SID 0x400), so two modules in boot mode at once would treat
  each other's ACKs as control frames. Fix: `C1FLTOBJ0` SID = 0 with `C1MASK0L = 0xFF`,
  `C1MASK0H |= 0x07`.
- Boot-block protection covers only half the bootloader: `BBSIZE_512` (512 words = 0x000–0x3FF)
  but the bootloader occupies 0x000–0x7FF. `BBSIZE_1024` matches the actual size and the
  `flushFlash()` software check (`>= 0x0800`). Must be changed in both `hwsettings.c` and
  `vlcb.c` so the two config images agree.
- `writeFlashByte()` (Q83) starts a READPAGE and immediately clears NVMCMD and restores NVMADR
  without waiting for GO; the library's `nvm.c` waits. Add the `while (NVMCON0bits.GO);`.
- All four read loops are `w <= READ_BYTES_QTY` — nine bytes instead of eight. Harmless on the
  Q83 (16-byte payload objects) but the address is advanced one too far; should be `<`.
- `C1CONT = 0x50` requests Normal FD mode mid-configuration (same as the library before b38);
  and `OSCCON1bits.NOSC = 2` is not followed by a wait for `OSCCON3bits.ORDY`, so the CAN
  peripheral is started before the PLL has locked. Both cosmetic in practice.
- `hwsettings.h` CANPAN3 `WPUA = 0b00101000` still enables the RA5 pull-up that upstream 5a13
  removed from the application; harmless for the few ms the bootloader runs.
- Self-verify is inactive: `main.c` defines `SELF_VERIFY` but the code tests `MODE_SELF_VERIFY`
  (commented out in `main.h`). Even enabled, the Q83 path only re-reads the page and compares
  nothing (the K80 path compares). Since `CHK_RUN`'s checksum is over the bytes *as received*,
  a failed page write is invisible to the client. A 256-byte `TBLRD` compare after each page
  write (~100 µs, ~40 bytes of code) would turn that into a NAK / NOK. Best-value bootloader change.
- Download speed is set by the client (per-frame ACK round trip and PC/USB latency), not the
  bootloader, which buffers and ACKs in microseconds and pays for one page erase/write per
  256 bytes. Only module-side saving: `flushFlash()` is called on every control frame; flushing
  only on a page change (already handled in `writeFlashByte()`) and on `CHK_RUN`/`RESET` would
  avoid an erase/write pair per address change — a flash-wear improvement more than a speed one.
- The unified hex carries two EEPROM records: `0x3803FA = 0xFF` (NVM version byte) and
  `0x3803FF = 0x00` (boot flag). They are only sent when "include EEPROM" is ticked in the
  MMC/FCU bootload dialog, and then the `0xFF` version byte makes the application factory-reset
  on its next start. This is intended: tick EEPROM only when changing module type or recovering
  a corrupt module; leave it unticked for a normal update and events/NVs/NN are kept.

## Upstream 5a13 — spikyian fa5f96b, 4 Aug 2026 — baseline

- RA5 set to output low as unused; WPUA 0b00101000 → 0b00001000. (Merged into this fork in
  5a34.) RA4 had already been driven low since upstream 5a11.
- Otherwise identical to 5a12 (6c32145, 22 May 2026), which added Keith Bruce's change
  allowing a flash ON event to turn other LEDs off (4-aspect signals).
