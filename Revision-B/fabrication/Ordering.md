# Prototype fabrication

Use the matching Gerber ZIP for each board. Each ZIP contains four copper layers, the outline, both masks, silkscreen and paste layers, plated and non-plated drills, and the Gerber job file. Specify four-layer, 1.6 mm FR-4. Preserve the cutouts and keyed connector orientation. Use the separate placement CSV and assembly tab of the workbook for each board. The placement coordinates come directly from KiCad in millimeters. Check supplier rotation conventions and pin-1 orientation in the assembly preview, especially on the bottom side.

Use fitted components on both faces as specified. The 32 optical windows face the floor. Protect the windows from residue. Use KiCad's board view to verify every QRE footprint orientation. Inspect protruding leads and fastener heads against the roughly 1.4 mm modeled floor margin. The motor driver modules and power wiring mount outside the controller PCB.

Read Build-and-review.md and resolve the chassis and plugged-harness fit before ordering both sets.
