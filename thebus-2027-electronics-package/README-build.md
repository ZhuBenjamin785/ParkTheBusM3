# TheBus 2027 Soccer Vision electronics, revision A

This package is for **two identical robots**. It contains a routed four-layer main-control board and a routed four-layer line-sensor ring, their editable KiCad projects, Gerber/drill ZIPs, STEP fit models, and a two-robot purchase spreadsheet. The boards are a modular **logic/sensing design**: the five Pololu G2 motor drivers, battery, fuses, switch, and 5 V regulator mount separately and are joined by a fused wire harness. No motor current flows through either PCB.

## Files

| Purpose | File |
| --- | --- |
| Main KiCad project | `pcbs/main-control/main-control.kicad_pro` |
| Line KiCad project | `pcbs/line-ring/line-ring.kicad_pro` |
| Main manufacturing ZIP | `fabrication/main-control-gerbers-drill.zip` |
| Line manufacturing ZIP | `fabrication/line-ring-gerbers-drill.zip` |
| Main and line fit models | `cad/main-control-board.step`, `cad/line-ring-board.step` |
| Board/component STEP previews | `cad/main-control-assembly.step`, `cad/line-ring-assembly.step` |
| Parts and purchase links | `thebus-two-robot-parts.xlsx` |

Both PCBs have four copper layers and specify 1.6 mm nominal FR-4. KiCad ERC found **0 violations** on each schematic. KiCad PCB DRC with schematic parity found **0 violations, 0 unconnected items, and 0 parity issues** on each routed board, using 0.15 mm minimum track/clearance and 0.20 mm copper-to-edge clearance. These are CAD checks, not physical or firmware validation.

## Geometry and fit

The main board outline was reconstructed from the user-provided Onshape FeatureScript: an approximately 120 mm circular outline with four 3 mm corner mounting holes and a right-side cutout. The 2.2 mm circles in that FeatureScript are **construction geometry**, so they were not drilled. The line ring outer contour and large mechanical cutouts/holes were extracted from `Line PCB Outline.zip` STEP; tiny holes belonging to the previous component pattern were omitted for the new sensor layout. Original CAD and synced project sources were not edited.

Import the two `*-board.step` files into the Onshape robot assembly and check the board face, mounting-hole alignment, 25D motor/driver clearance, battery bay, cable bends, N6 view, and total 180 mm Soccer Vision envelope. The KiCad STEP exporter puts the board near its PCB editor origin and may show an approximately 1.51 mm solid despite the 1.6 mm board property; use **1.6 mm** for the physical stack and hole/standoff selection. The robot assembly was only available as Parasolid, so the complete mounting stack and actual line-sensor floor gap were not measurable here. Place the QTRX sensing faces about **10 mm above the floor** on separately made brackets, then tune on the competition floor.

## Power and interconnect

```
3S XT60 battery
  ├─ 1 A fused always-on feed → referee module VIN (GND shared)
  └─ 30 A fuse near battery → 30 V DC / 30 A emergency switch
        ├─ five 7.5 A fused branches → G2 VIN (one driver per motor)
        └─ 2 A fused branch → Pololu 5 V regulator
              ├─ main board J13 pin 1 +5V, pin 2 GND
              └─ OpenMV N6 VIN +5V, GND common
```

Fit a 470 µF, at least 25 V capacitor across VIN/GND at each G2 driver as a starting point. Connect each driver's OUTA/OUTB directly to its motor power leads; connect PWM, DIR, SLP, CS, FLT and GND to the corresponding main-board J3–J7 logic connector according to `connections.csv`. Solder a **local 1 kΩ SLP-to-GND pull-down at every G2 board** so loss of the logic cable keeps that driver asleep. The main board drives the common SLP line only when referee PLAY and the Teensy MCU_ENABLE output are both high. With all five drivers connected, measure SLP above 2.3 V in PLAY and below 0.3 V in STOP and after unplugging the logic harness from an individual driver; the six 1 kΩ pull-downs load the gate output, so this bench check is mandatory. Test STOP, E-stop, reset, and disconnected-harness behavior with wheels off the floor. Limit acceleration, monitor CS, and enforce a stall timeout in firmware; the G2 default current limit is too high to protect a 5 A-stall motor by itself.

Each 25D motor has a six-lead cable: red/black are **motor power**, while blue is encoder VCC, green encoder GND, yellow encoder A and white encoder B. Split the original 1×6 lead into a separate motor-power pair and a 4-pin JST XH logic plug. J8–J12 are **pin 1 +5V, pin 2 GND, pin 3 A, pin 4 B**. The board's two TI SN74LVC245A chips translate the ten encoder signals to the Teensy's 3.3 V GPIO. Never apply the encoder outputs directly to the Teensy. The main PCB's battery monitor J18 must connect to the fused battery positive and GND, not the 5 V rail.

The line board's QTRX-MD-01A-S sensor ports J2–J17 follow Pololu's connector order: **pin 1 OUT, pin 2 VCC (3.3 V), pin 3 GND, pin 4 CTRL**. Use Pololu 4-pin JST SH cables and continuity-check pin numbers on **both ends** before plugging in all 16 sensors. The board J1 and main J16 have matching 8-signal names; build their XH harness pin-for-pin. The CD74HC4067M multiplexes 16 analog outputs to Teensy LINE_ANALOG; select pins S0–S3 are binary channel address, and LINE_CTRL controls all emitters. Software must allow settling after each channel switch and calibrate against white field/black line.

The N6 receives 5 V at its own VIN, separately from J14. J14 carries GND, Teensy TX1 to **N6 P5/RX**, and N6 **P4/TX** to Teensy RX1. Do not feed 5 V into N6 GPIO. The Teensy is supplied at VIN through J13; follow the Teensy manufacturer's VUSB/VIN isolation instruction before attaching a USB programming cable at the same time as external 5 V.

## Assembly and commissioning gates

1. Import STEP fit models into Onshape, set the actual board planes/standoff lengths, verify every keepout and connector access, and recheck the 180 mm robot envelope. Model the selected 25D motors, wheels, hubs, N6, switch, battery, and fused harness before committing to fabrication.
2. Buy the spreadsheet's selected modules and quote the unpriced connectors, PCB fabrication, power wiring and hardware. The price subtotal includes only rows with a verified unit price; it is **not a total build price**. OpenMV N6, some Pololu items, and referee module procurement may involve backorders or organizers.
3. Assemble one main PCB and one line PCB first. Measure 5 V and 3.3 V rails, confirm all encoder outputs remain within 3.3 V, then connect Teensy and camera.
4. Attach sensors one at a time and verify their pin order and 16 distinct mux readings. Attach drivers one at a time with current-limited bench power before using the LiPo. Validate reversed wheel direction and current sense.
5. Implement/test OpenMV N6 vision and Teensy motion/referee firmware. The supplied Raspberry Pi `BallAlgo` code and referenced `Offense2026` Teensy firmware are **not drop-in software** for this architecture. The baseline has no powered kicker; it can move and dribble, while a kicker needs a specified actuator and dedicated driver before connection to J19 AUX logic.

The [2027 draft Soccer rules](https://robocup-junior.github.io/soccer-rules/2027-soccer-draft-rules/rules.html) call the former Open division **Soccer Vision**, set an 18 cm robot envelope, and describe a referee communication module. Confirm the final 2027 rules and local U.S. event variations before inspection. Hardware details are based on [Pololu 25D encoder wiring](https://www.pololu.com/file/0J1829/pololu-25d-metal-gearmotors.pdf), the [Pololu QTRX pin schematic](https://www.pololu.com/file/0J1558/qtr-qtrx-qtrxl-hd-md-reflectance-sensor-array-schematic.pdf), [TI SN74LVC245A ratings](https://www.ti.com/lit/ds/symlink/sn74lvc245a.pdf), and the [OpenMV N6 pin reference](https://docs.openmv.io/dev/openmvcam/quickref/openmv-n6.html).
