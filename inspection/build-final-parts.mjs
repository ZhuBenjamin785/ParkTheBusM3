import fs from 'node:fs/promises';
import {Workbook, SpreadsheetFile} from '@oai/artifact-tool';

const root = new URL('../', import.meta.url).pathname;
const wb = Workbook.create();
const order = wb.worksheets.add('Two robot purchase list');
const board = wb.worksheets.add('PCB assembly BOM');
const guide = wb.worksheets.add('Wiring and checks');
const p = (id,group,part,qty,usd,url,notes,status='Selected') => [id,group,part,qty,usd,null,url,notes,status];
const rows = [
 p('P01','Control','Teensy 4.1, DEV-16771',2,31.50,'https://www.sparkfun.com/teensy-4-1.html','One per robot; solder compatible pin headers.'),
 p('P02','Vision','OpenMV N6 camera',2,195,'https://openmv.io/products/openmv-n6','Remote mount; 5 V supply and 3.3 V UART; lens/mirror field of view must be tested.'),
 p('P03','Drive','Pololu 25D 9.7:1 HP 12 V encoder motor #4842',8,56.95,'https://www.pololu.com/product/4842','Four drive motors per robot; 4 mm D shaft, 5 A stall.'),
 p('P04','Dribbler','Pololu 25D 4.4:1 HP 12 V encoder motor #4841',2,56.95,'https://www.pololu.com/product/4841','One per robot; roller and coupling to be designed.'),
 p('P05','Drive','Pololu G2 18v17 motor driver #2991',10,44.95,'https://www.pololu.com/product/2991','Five per robot; external high-current mounts, not on control PCB.'),
 p('P06','Power','Pololu D42V55F5 5 V regulator #5571',2,39.95,'https://www.pololu.com/product/5571','One per robot; feeds controller, camera and encoders.'),
 p('P07','Navigation','Pololu MinIMU-9 v6 #2862',2,29.95,'https://www.pololu.com/product/2862','3.3 V I2C; mount away from motor wiring.'),
 p('P08','Line sensing','Pololu QTRX-MD-01A-S analog #4440',32,4.29,'https://www.pololu.com/product/4440','Sixteen per robot; 10 mm floor gap target; qty-25 discount may apply.'),
 p('P09','Line sensing','Pololu 4-pin JST SH cable 10 cm #5523',32,2.31,'https://www.pololu.com/product/5523','One per sensor; continuity-check pin order; listed with backorders.'),
 p('P10','Drive','Nexus 48 mm aluminum double omni wheel #14148',8,12,'https://ozrobotics.com/shop/aluminum-double-omni-wheel-basic-48mm-14148/','Four per robot; check 18 cm diameter envelope.'),
 p('P11','Drive','Nexus 4 mm hub for 48 mm wheel #18023',8,6.90,'https://ozrobotics.com/shop/4mm-aluminum-hub-for-48mm-aluminum-omni-wheel-18023/','One per wheel; 4 mm motor output shaft.'),
 p('P12','Mounting','Pololu 25D motor bracket pair #2676',5,10.95,'https://www.pololu.com/product/2676','Ten brackets total for ten motors; verify against existing chassis mounts.'),
 p('P13','Power','Gens ace 3S 2200 mAh 35C XT60 LiPo',4,22.69,'https://www.horizonhobby.com/product/11.1v-2200mah-3s-35c-g-tech-bashing-lipo-battery-xt60/GEA223S35X6GT.html','Two fitted plus two spares; bay fit and mass must be measured.'),
 p('P14','Power','3S balance charger, Gens ace IMARS D300',1,null,'https://www.horizonhobby.com/product/imars-d300-g-tech-ac-dc-300w-700w-charger-white/GEA300WD300UW.html','Shared team charger; quote current U.S. price.','Quote'),
 p('P15','Rules','Official RCJ soccer referee communication module',2,null,'https://github.com/robocup-junior/soccer-communication-module','One per robot; ask U.S. organizer about 2027 supply and revision; keep powered while drive is disabled.','Organizer'),
 p('P16','Fabrication','Main control PCB, 4-layer 1.6 mm',2,null,'https://jlcpcb.com/','Upload main-control Gerber ZIP; fit review before ordering.','Quote'),
 p('P17','Fabrication','Line ring PCB, 4-layer 1.6 mm',2,null,'https://jlcpcb.com/','Upload line-ring Gerber ZIP; sensor carriers are separate.','Quote'),
 p('P18','PCB IC','TI SN74LVC245APWR TSSOP-20',4,null,'https://www.digikey.com/en/products/detail/texas-instruments/SN74LVC245APWR/276876','Two per main board; converts ten encoder outputs 5 V to 3.3 V.','Quote'),
 p('P19','PCB IC','TI SN74LVC1G08DBVR SOT-23-5',2,null,'https://www.digikey.com/en/products/detail/texas-instruments/SN74LVC1G08DBVR/385718','One per main board; referee PLAY and MCU-enable drive gate.','Quote'),
 p('P20','PCB IC','TI CD74HC4067M96 SOIC-24',2,null,'https://www.digikey.com/en/products/detail/texas-instruments/CD74HC4067M96/1507236','One per line ring, 16-channel analog mux.','Quote'),
 p('P21','PCB connector','JST SH SM04B-SRSS-TB 4-pin SMD',32,null,'https://www.digikey.com/en/products/detail/jst-sales-america-inc/SM04B-SRSS-TB/926710','Sixteen per line ring.','Quote'),
 p('P22','PCB connector','JST XH B2B-XH-A 2-pin header',6,null,'https://www.digikey.com/en/products/result?s=N4IgTCBcDaIBwE4CsBaAbANgOQCIAMYBLAOwBdAXyA','Three per main board; verify exact footprint variant.','Quote'),
 p('P23','PCB connector','JST XH B3B-XH-A 3-pin header',2,null,'https://www.digikey.com/en/products/result?s=N4IgTCBcDaIBwE4CsBaAbANgOQCIAMYBLAOwBdAXyA','One camera UART header per main board.','Quote'),
 p('P24','PCB connector','JST XH B4B-XH-A 4-pin header',14,null,'https://www.digikey.com/en/products/result?s=N4IgTCBcDaIBwE4CsBaAbANgOQCIAMYBLAOwBdAXyA','Seven per main board.','Quote'),
 p('P25','PCB connector','JST XH B6B-XH-A 6-pin header',10,null,'https://www.digikey.com/en/products/result?s=N4IgTCBcDaIBwE4CsBaAbANgOQCIAMYBLAOwBdAXyA','Five G2 logic headers per main board.','Quote'),
 p('P26','PCB connector','JST XH B8B-XH-A 8-pin header',4,null,'https://www.digikey.com/en/products/result?s=N4IgTCBcDaIBwE4CsBaAbANgOQCIAMYBLAOwBdAXyA','One line header on each board per robot.','Quote'),
 p('P27','Cable mating','JST XHP-2/3/4/6/8 mating housings',36,null,'https://www.jst-mfg.com/product/index.php?series=277','Per 2 robots: housings 6/2/14/10/4; buy each size separately.','Quote'),
 p('P28','Controller socket','1x24 2.54 mm female socket strip',4,null,'https://www.digikey.com/en/products/filter/rectangular-connectors-headers-receptacles-female-sockets/315','Two per Teensy, verify 2.54 mm row spacing and header height.','Quote'),
 p('P29','PCB passives','0805 resistor and capacitor set',1,null,'https://www.digikey.com/en/products/filter/resistors/chip-resistor-surface-mount/52','Exact values/counts in PCB assembly sheet; buy a workshop assortment or individual reels.','Quote'),
 p('P30','Power wiring','XT60 mating plugs, 14 AWG and 18 AWG silicone wire',2,null,'https://www.amass.com.cn/en/products/xt60.html','Two full robot harness sets; motor current never through logic PCB.','Quote'),
 p('P31','Power wiring','NKK SW3821D/328 30 A 30 V DC switch',2,49.28,'https://www.digikey.com/en/products/detail/nkk-switches/SW3821D-328/4512701','DPST DC-rated emergency disconnect, accessible panel mount; CAD mounting and lead time to check.'),
 p('P32','Protection','Littelfuse 0287030.L 30 A ATO fuse',2,null,'https://www.digikey.com/en/products/detail/littelfuse-inc/0287030-L/3672102','One per robot near battery; final size must follow measured load and wiring.','Quote'),
 p('P33','Protection','Littelfuse 028707.5L 7.5 A ATO fuse',10,0.68,'https://www.digikey.com/en/products/detail/littelfuse-inc/028707-5L/3672105','Five motor branches per robot; verify stall time and holder sizing.'),
 p('P34','Protection','Littelfuse 0287002 2 A ATO logic fuse',2,null,'https://www.digikey.com/en/products/detail/littelfuse-inc/0287002-PXCN/3102552','One protected logic regulator feed per robot.','Quote'),
 p('P35','Protection','470 uF >=25 V low-ESR bulk capacitor',10,null,'https://www.digikey.com/en/products/filter/aluminum-electrolytic-capacitors/58','One near each G2 VIN/GND as a starting point; verify driver guide.','Quote'),
 p('P36','Protection','1 kΩ pull-down resistor on each G2 SLP pin',10,null,'https://www.digikey.com/en/products/filter/through-hole-resistors/53','Install at each driver so disconnected logic harness leaves driver asleep.','Quote'),
 p('P37','Mechanical','M3 standoffs, screws, locknuts and sensor brackets',2,null,'https://www.mcmaster.com/m3-standoffs/','Two hardware sets; board and floor spacing must be measured on assembly.','Measure'),
 p('P38','Programming','USB cables for Teensy and OpenMV',4,null,'https://www.sparkfun.com/categories/17','Two of each connection type; confirm exact plugs and service access.','Verify plugs'),
 p('P39','Battery safety','LiPo charging/storage safety bag',2,null,'https://www.horizonhobby.com/search?q=lipo%20bag','One bag for two packs or one per robot kit; verify capacity and local event rules.','Quote'),
 p('P40','Protection','Littelfuse 0287001 1 A ATO referee fuse',2,null,'https://www.digikey.com/en/products/detail/littelfuse-inc/0287001-PXCN/3102551','Separate always-on module feed, one per robot.','Quote'),
 p('P41','Protection','Littelfuse 01550400Z 30 A ATO fuse holder',2,7.24,'https://www.digikey.com/en/products/detail/littelfuse-commercial-vehicle-products/01550400Z/146699','10 AWG lead holder for P32, one per robot.'),
 p('P42','Protection','Littelfuse 01550300Z 20 A ATO fuse holder',14,null,'https://www.digikey.com/en/products/detail/littelfuse-commercial-vehicle-products/01550300Z/146696','14 AWG lead holder for ten motor plus four logic/referee fuses.','Quote'),
 p('P43','Cable mating','JST SXH-001T-P0.6 crimp contacts',200,null,'https://www.jst-mfg.com/product/index.php?series=277','166 contacts required across two robots plus 34 spares; correct XH crimp tool required.','Quote'),
];
function style(ws,last,cols){
 ws.showGridLines=false;
 ws.getRange(`A1:${cols}${last}`).format.font={name:'Arial',size:10,color:'#203246'};
 ws.getRange(`A1:${cols}${last}`).format.verticalAlignment='center';
 ws.getRange(`A1:${cols}${last}`).format.wrapText=true;
 ws.getRange(`A7:${cols}7`).format={fill:'#203B58',font:{name:'Arial',size:10,bold:true,color:'#FFFFFF'},wrapText:true};
 ws.getRange(`A8:${cols}${last}`).format.rowHeight=40;
 ws.freezePanes.freezeRows(7);
}
style(order,7+rows.length,'I');
for(const [c,w] of Object.entries({A:8,B:16,C:43,D:11,E:13,F:15,G:64,H:70,I:17}))order.getRange(`${c}1:${c}${7+rows.length}`).format.columnWidth=w;
order.getRange('A2').values=[['THEBUS — two-robot purchase list']];
for(const r of [2,3,4])order.getRange(`A${r}:I${r}`).merge();
order.getRange('A2:I2').format.rowHeight=32;
order.getRange('A3:I4').format.rowHeight=28;
order.getRange('A2').format.font={name:'Arial',size:17,bold:true,color:'#203B58'};
order.getRange('A3').values=[['2027 RoboCup Junior Soccer Open target. Prices checked October 10, 2026; refresh before paying. Quantities cover two robots and listed spares.']];
order.getRange('A4').values=[['Priced rows subtotal only; unpriced hardware, fabrication, tax, shipping and backorders remain additional.']];
order.getRange('D5').values=[['Priced subtotal USD']];
order.getRange('F5').formulas=[[`=SUM(F8:F${7+rows.length})`]];
order.getRange('F5').setNumberFormat('$0.00');
order.getRange('A7:I7').values=[['ID','System','Product / exact SKU','Qty for 2','Unit USD','Extended USD','Purchase / vendor link','Fit or wiring note','Status']];
order.getRange(`A8:I${7+rows.length}`).values=rows;
order.getRange(`F8:F${7+rows.length}`).formulas=rows.map((_,i)=>[`=IF(ISNUMBER(E${8+i}),D${8+i}*E${8+i},"")`]);
order.getRange(`E8:F${7+rows.length}`).setNumberFormat('$0.00');
order.getRange(`G8:G${7+rows.length}`).format.font={name:'Arial',size:10,color:'#145CA8',underline:'single'};
order.getRange(`I8:I${7+rows.length}`).format.fill='#FFF0C8';
order.tables.add(`A7:I${7+rows.length}`,true,'TwoRobotPurchases');

const assy=[
 ['Main','J1-J2','1x24 female socket',4,'Teensy 4.1; 2 per board'],
 ['Main','J3-J7','JST XH 6-pin',10,'G2 logic; 5 per board'],
 ['Main','J8-J12','JST XH 4-pin',10,'Motor encoder 5 V; 5 per board; split 6-lead motor harness'],
 ['Main','J13','JST XH 2-pin',2,'5 V regulator logic input'],
 ['Main','J14','JST XH 3-pin',2,'OpenMV UART'],
 ['Main','J15','JST XH 4-pin',2,'MinIMU I2C'],
 ['Main','J16','JST XH 8-pin',2,'Line board harness'],
 ['Main','J17-J18','JST XH 2-pin',4,'Referee PLAY and battery sense'],
 ['Main','J19','JST XH 4-pin',2,'Auxiliary logic only'],
 ['Main','U1','SN74LVC1G08DBVR',2,'Referee/MCU AND gate'],
 ['Main','U2-U3','SN74LVC245APWR',4,'5 V tolerant encoder buffers at 3.3 V'],
 ['Main','R1,R2,R4','0805 10 kΩ',6,'Pulls/control'],
 ['Main','R3','0805 1 kΩ',2,'Driver gate pull-down'],
 ['Main','R5','0805 100 kΩ 1%',2,'Battery divider top'],
 ['Main','R6','0805 27 kΩ 1%',2,'Battery divider bottom'],
 ['Main','C1,C2,C5,C6','0805 100 nF 16 V',8,'Decoupling/filter'],
 ['Main','C3,C4','0805 10 uF 10 V',4,'Supply bypass'],
 ['Line','U1','CD74HC4067M96',2,'16-channel analog mux'],
 ['Line','J1','JST XH 8-pin',2,'Main board harness'],
 ['Line','J2-J17','JST SH SM04B-SRSS-TB',32,'QTRX sensor ports'],
 ['Line','R1-R4','0805 100 kΩ',8,'Mux select pull-downs'],
 ['Line','R5','0805 10 kΩ',2,'Emitter-control pull-up'],
 ['Line','C1','0805 100 nF 16 V',2,'Mux bypass'],
 ['Line','C2','0805 10 uF 10 V',2,'Supply bypass'],
 ['Harness','XHP-2/3/4/6/8','JST XH mating housings',36,'Counts 6/2/14/10/4 across two robots'],
 ['Harness','SXH-001T-P0.6','JST XH crimp contacts',200,'166 required plus spare; use proper crimp tooling'],
];
style(board,7+assy.length,'E');
for(const [c,w] of Object.entries({A:17,B:20,C:42,D:15,E:75}))board.getRange(`${c}1:${c}${7+assy.length}`).format.columnWidth=w;
board.getRange('A2').values=[['PCB assembly counts for two robots']];
board.getRange('A3').values=[['Values and package footprints match the supplied KiCad schematic and PCB files. Check polarity and connector orientation before soldering.']];
board.getRange('A2:E2').merge();board.getRange('A3:E3').merge();
board.getRange('A2:E2').format.rowHeight=32;board.getRange('A3:E3').format.rowHeight=28;
board.getRange('A7:E7').values=[['Board','References','Value / package','Qty','Assembly note']];
board.getRange(`A8:E${7+assy.length}`).values=assy;
board.tables.add(`A7:E${7+assy.length}`,true,'BoardAssembly');

const checks=[
 ['1','Before fabrication','Import both board STEP files into Onshape; verify holes, cutouts, 1.6 mm thickness, all component and cable clearances. Main FeatureScript outline is reconstructed, not an Onshape-native export.'],
 ['2','Power tree','3S XT60 -> 30 A main fuse -> 30 A DC emergency disconnect -> five 7.5 A motor fuses and separate 2 A logic branch. Referee module needs its own protected always-on feed.'],
 ['3','High current','Each G2 VIN/GND is wired directly to fused battery branches with suitable wire. G2 OUTA/OUTB feed motor red/black. 470 uF >=25 V at each driver. Main PCB carries logic only.'],
 ['4','Safe enable','Referee OUT1 PLAY -> J17 REF_PLAY. Teensy MCU_ENABLE must remain low until ready. Each G2 SLP also needs a local 1 kΩ pull-down; test harness-disconnect stopping.'],
 ['5','Encoder','25D blue encoder VCC -> J8-J12 pin 1 (5 V); green GND -> pin 2; yellow A -> pin 3; white B -> pin 4. Red/black motor leads do not enter the main PCB.'],
 ['6','Line sensors','QTRX pin order from Pololu schematic: pin 1 OUT, 2 VCC, 3 GND, 4 CTRL; J2-J17 match it. Continuity-check each 4-wire cable and mount sensing faces approximately 10 mm above floor.'],
 ['7','Camera/firmware','OpenMV N6 gets regulated 5 V at VIN separately; J14 carries GND, N6 P5 RX from Teensy TX1 and N6 P4 TX to Teensy RX1. Isolate Teensy VUSB from VIN during simultaneous USB/external power. Existing Pi BallAlgo/Teensy code needs porting.'],
 ['8','Bench release','Confirm polarity, supply rails, encoder 3.3 V outputs, referee STOP, emergency switch, 16 sensor readings, each motor direction and current, then wheel/ball vision calibration.'],
];
style(guide,7+checks.length,'C');
for(const [c,w] of Object.entries({A:10,B:27,C:120}))guide.getRange(`${c}1:${c}${7+checks.length}`).format.columnWidth=w;
guide.getRange('A2').values=[['Wiring and validation notes']];
guide.getRange('A2:C2').merge();guide.getRange('A2:C2').format.rowHeight=32;
guide.getRange('A7:C7').values=[['Step','Area','Instruction']];
guide.getRange(`A8:C${7+checks.length}`).values=checks;
guide.getRange(`A8:C${7+checks.length}`).format.rowHeight=74;
guide.tables.add(`A7:C${7+checks.length}`,true,'BuildChecks');
wb.recalculate();
console.log((await wb.inspect({kind:'match',searchTerm:'#REF!|#DIV/0!|#VALUE!|#NAME\\?|#N/A|#NUM!|#NULL!',options:{useRegex:true,maxResults:50}})).ndjson);
console.log((await wb.inspect({kind:'table',range:`Two robot purchase list!D5:F${7+rows.length}`,include:'values,formulas',tableMaxRows:6,tableMaxCols:3})).ndjson);
for(const [ws,range,file] of [[order,'A1:I20','purchase-top'],[order,`A21:I${7+rows.length}`,'purchase-bottom'],[board,'A1:E18','assembly']]){
 const blob=await wb.render({sheetName:ws.name,range,scale:1,format:'png'});
 await fs.writeFile(`${root}inspection/${file}.png`,new Uint8Array(await blob.arrayBuffer()));
}
await (await SpreadsheetFile.exportXlsx(wb)).save(`${root}thebus-two-robot-parts.xlsx`);
