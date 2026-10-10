import fs from 'node:fs/promises';
import {Workbook, SpreadsheetFile} from '@oai/artifact-tool';
const out = new URL('../', import.meta.url).pathname;
const wb = Workbook.create();
const options = wb.worksheets.add('Proposed purchases');
const parts = wb.worksheets.add('Parts to confirm');
function base(s, end, widths) {
  s.showGridLines = false;
  s.getRange(`A1:H${end}`).format.font = {name:'Arial',size:11,color:'#202A36'};
  s.getRange(`A1:H${end}`).format.verticalAlignment = 'center';
  s.getRange('A2').format.font = {name:'Arial',size:16,bold:true};
  s.getRange(`A6:H${end}`).format.rowHeight = 44;
  widths.forEach((w,i)=>s.getRange(`${String.fromCharCode(65+i)}1:${String.fromCharCode(65+i)}${end}`).format.columnWidth=w);
  s.getRange('A6:H6').format = {fill:'#253D5A',font:{name:'Arial',size:11,bold:true,color:'#FFFFFF'},wrapText:true,horizontalAlignment:'center',verticalAlignment:'center'};
  s.freezePanes.freezeRows(6);
  s.freezePanes.freezeColumns(2);
}
const rows = [
 ['P01','Main controller',1,'Provisional','Teensy 4.1 in existing schematic','Confirm retained; supply Offense2026 firmware','C01','thebus/thebus.kicad_sch: U1'],
 ['P02','OpenMV camera',1,'Model needed','OpenMV specified by user','Exact model/revision, lens and existing ownership','C02 or C03 are examples','User request'],
 ['P03','Drive motors',null,'Model needed','Four-wheel software model only','Motor SKU, count, rated voltage, stall current, shaft','Pending exact motor','BallAlgo/docs/pipeline_context.md'],
 ['P04','Drive motor drivers',null,'Model needed','One channel per brushed motor','Match battery, stall/load current, braking and heat','C04 or C05 alternatives','Candidate architecture'],
 ['P05','Battery',1,'Specification needed','Software uses 12 V model','Actual chemistry, cell count, capacity, connector','Pending pack selection','BallAlgo/params.json is not a battery specification'],
 ['P06','Battery charger',1,'Specification needed','Shared workshop item if needed','Must match pack chemistry, cell count and charge rate','Pending pack selection','Candidate support equipment'],
 ['P07','5 V regulator',1,'Provisional','Logic/camera supply candidate','Load budget and minimum battery voltage under load','C06','Candidate architecture'],
 ['P08','Raspberry Pi + carrier',null,'Scope needed','CM5 appears in supplied software','Retain Pi or replace vision/control architecture?','Pending decision','BallAlgo/README.md'],
 ['P09','Dribbler motor + driver',null,'Scope needed','Dribbler commands exist','Motor type, voltage, current, count and driver type','Pending motor selection','BallAlgo/params.json'],
 ['P10','Kicker + driver circuit',null,'Scope needed','Kicker appears in protocol/docs','Coil voltage/current, pulse duration and energy source','Pending actuator specification','BallAlgo/docs/CODE_REVIEW_FINDINGS.md'],
 ['P11','Boundary/line sensors',null,'Scope needed','Line sensing appears in docs','Count, ring geometry, ground clearance and output type','C07 is one candidate','BallAlgo/docs/CODE_REVIEW_FINDINGS.md'],
 ['P12','IMU / heading sensor',null,'Model needed','IMU used by existing software','Exact module, bus, voltage and magnetic environment','Pending sensor selection','BallAlgo/docs/pipeline_context.md'],
 ['P13','Optical odometry sensor',null,'Scope needed','Mouse fusion enabled in config','Keep sensor? Exact module, bus and mounting height','Pending sensor selection','BallAlgo/src/config.hpp'],
 ['P14','LiDAR + cable',null,'Scope needed','LD19 support in source','Retain LD19? Confirm module and connector pinout','Pending decision','BallAlgo/src/lidar/Ld19Reader.cpp'],
 ['P15','Encoders + cables',null,'Scope needed','Not established by motor model','Motor encoder option and signal voltage/output type','Pending motor selection','No confirmed encoder specification'],
 ['P16','Fuse, holder, disconnect',null,'Ratings needed','Power entry protection','Size to harness, pack fault current and load/inrush','Pending current budget','Candidate architecture'],
 ['P17','Reverse-polarity / surge parts',null,'Ratings needed','Board input protection','Topology, voltage margin and braking transients','Pending circuit design','Candidate architecture'],
 ['P18','Connectors + mating harnesses',null,'Pinouts needed','Battery/motors/camera/sensors','Current rating, pin count, locking style, wire gauge','Pending circuit and cable plan','Candidate architecture'],
 ['P19','Teensy/module headers',null,'Mounting needed','Socketed modules proposed','Header/socket heights and accessible underside pins','Pending module mounting','Candidate architecture'],
 ['P20','Passives + signal protection',null,'Circuit needed','Caps, resistors, level protection','Values, ratings, packages and quantities after schematic','Pending schematic BOM','Candidate architecture'],
 ['P21','Custom control/power PCB',null,'Dimensions needed','No board outline in source PCB','Outline, holes, stack height, copper/layers and assembly','Fabricator quote after layout','thebus/thebus.kicad_pcb is empty'],
 ['P22','Custom sensor PCB',null,'Scope needed','Only if ring board is required','Sensor placement, count, ground clearance and mounting','Fabricator quote after layout','Candidate architecture'],
 ['P23','Mounting hardware',null,'Dimensions needed','Standoffs, screws, insulation','Hole sizes, mounting stack and keepouts','Pending CAD dimensions','CAD exports require geometric verification'],
 ['P24','Programming/data cables',null,'Model needed','Camera and controller service access','USB connector type and access in assembled robot','Pending camera model','Candidate support equipment']
];
rows[0]=['P01','Main controller',1,'Selected baseline','Teensy 4.1','New firmware/pin assignment required','C01','Existing schematic plus selected architecture'];
rows[1]=['P02','OpenMV camera',1,'Selected baseline','OpenMV N6','Verify mirror/lens fit and UART wiring','C02','User delegated model selection'];
rows[2]=['P03','Drive motors',4,'Proposed; fit pending','Pololu 25D HP 12 V, 9.7:1, encoders','Verify 25 mm mounts, 4 mm shafts, weight and wheel fit','C03','Selected starting point, not confirmed CAD fit'];
rows[3]=['P04','Motor drivers',5,'Proposed; fit pending','G2 18v17: four drive + one dribbler','Set current limits; verify thermal layout and wiring','C05','Selected starting point'];
rows[4]=['P05','Battery packs',2,'Proposed; fit pending','3S 2200 mAh XT60: one fitted, one spare','Verify battery bay and competition voltage limit','C09','11.1 V nominal; 12.6 V fully charged'];
rows[5]=['P06','Battery charger',1,'Proposed','Gens ace IMARS D300 US AC model','One shared charger; verify supplied balance leads','C10','Exact price unavailable in retrieved page'];
rows[6]=['P07','5 V regulator',1,'Selected baseline','Pololu D42V55F5','Verify stock, load/thermal budget and connector drop','C06','6 A headline depends on voltage/cooling'];
rows[7]=['P08','Optional Linux host',null,'Architecture tradeoff','Existing BallAlgo requires a Pi','Baseline uses N6 + Teensy; Pi software is not drop-in','Not included in subtotal','A Linux host is required if retaining BallAlgo'];
rows[8]=['P09','Dribbler motor',1,'Proposed; fit pending','Pololu 25D HP 12 V, 4.4:1, encoder','Verify roller speed, 4 mm coupling and packaging','C04; driver counted in P04','Selected starting point'];
rows[10]=['P11','Boundary sensors',16,'Proposed; fit pending','Pololu QTRX-HD-01A at 3.3 V','Verify ground clearance, ring diameter and line coverage','C07','Count is a design assumption'];
rows[11]=['P12','IMU / heading sensor',1,'Selected baseline','Pololu MinIMU-9 v6','Mount away from motor currents; calibrate heading','C08','Power/interfaces at 3.3 V'];
rows[12]=['P13','Optical odometry',null,'Not in baseline','Wheel encoders proposed initially','Add ground tracking if wheel-slip testing requires it','Not included','Existing mouse-fusion software must be adapted'];
rows[13]=['P14','LiDAR',null,'Not in baseline','Camera + encoders + IMU proposed','Pi/LD19 localization would require extra integration','Not included','Existing LD19 software cannot simply be reused'];
rows[14]=['P15','Motor encoder cables',5,'Cable design pending','Encoders included in selected motors','Verify mating cables and 3.3 V signal conditioning','Included encoders; harness extra','Four drive motors plus dribbler'];
rows[23]=['P24','Programming/data cables',2,'Connector check pending','One camera and one Teensy USB data cable','Verify connector type and service access in robot','Cable SKUs pending','Required for firmware upload and debugging'];
rows.push(['P25','Omniwheels + hubs',4,'Fit pending','50 mm diameter assumed by software','Match 4 mm motor shafts; verify wheel envelope/load','Pending mechanical fit','BallAlgo/params.json: wheel_radius_m = 0.025']);
rows.push(['P26','Line-sensor acquisition circuit',1,'Circuit needed','16-channel analog multiplexer or ADC','Select IC, settling time, filtering and 3.3 V supply','Pending schematic BOM','Required for the proposed sixteen-sensor ring']);
base(parts,32,[8,30,12,23,39,54,30,53]);
parts.getRange('A2').values=[['THEBUS — preliminary parts list']];
parts.getRange('A3').values=[['Proposed one-robot system, U.S. purchasing. CAD fit and competition rules remain unverified. No fabrication-ready PCB has been produced.']];
parts.getRange('A4').values=[['Blank quantity means unresolved or excluded. Counts are design assumptions; batteries include one spare. Purchase IDs refer to Proposed purchases.']];
parts.getRange('A6:H6').values=[['ID','Part / subsystem','Qty basis','Current status','Evidence / proposed role','Information needed','Purchase reference','Source / provenance']];
parts.getRange('A7:H32').values=rows;
parts.getRange('A7:H32').format.wrapText=true;
parts.getRange('C7:C32').setNumberFormat('0');
parts.getRange('D7:D32').format.fill='#FFF1CF';
for(let r=7;r<=32;r+=2) parts.getRange(`E${r}:H${r}`).format.fill='#F1F4F8';
parts.tables.add('A6:H32',true,'PartsToConfirm');
const candidates=[
 ['C01','Teensy 4.1 / DEV-16771',31.50,1,null,'Main controller. Add headers/socket and USB power isolation in final board.','https://www.sparkfun.com/teensy-4-1.html','Listed in stock; recheck at checkout.'],
 ['C02','OpenMV N6 / OMV-N6-V1',195,1,null,'Color global shutter. Supply VIN with regulated 5 V; I/O at 3.3 V. Verify mirror geometry.','https://openmv.io/products/openmv-n6','Selected model; confirm lead time.'],
 ['C03','Pololu 25D 9.7:1 HP / #4842',56.95,4,null,'Four drive motors. 12 V, 1000 RPM, 5 A stall each; 4 mm shafts. CAD fit unverified.','https://www.pololu.com/product/4842','Active/preferred; backorders allowed.'],
 ['C04','Pololu 25D 4.4:1 HP / #4841',56.95,1,null,'Dribbler candidate. 12 V, 2200 RPM, 5 A stall. Roller speed and motor fit unverified.','https://www.pololu.com/product/4841','Active/preferred; backorders allowed.'],
 ['C05','Pololu G2 18v17 / #2991',44.95,5,null,'Four drive + one dribbler driver. Lower default current limit to protect motors; cooling check required.','https://www.pololu.com/product/2991','Active/preferred; backorders allowed.'],
 ['C06','Pololu D42V55F5 / #5571',39.95,1,null,'5 V logic supply. Check thermal/load budget and voltage at camera; reserve separate supply for any Pi.','https://www.pololu.com/product/5571','Rationed active/preferred; confirm supply.'],
 ['C07','Pololu QTRX-HD-01A / #4401',3.75,16,null,'Proposed 16-point line ring, 3.3 V supply. Optimal height 10 mm; coverage requires CAD and testing.','https://www.pololu.com/product/4401','Active/preferred; backorders allowed.'],
 ['C08','Pololu MinIMU-9 v6 / #2862',29.95,1,null,'3.3 V I2C and supply. Mount away from motors; compass reliability needs testing.','https://www.pololu.com/product/2862','Active/preferred; recheck stock.'],
 ['C09','Gens ace 3S 2200 / GEA223S35X6GT',22.69,2,null,'One 11.1 V nominal, 12.6 V full pack in robot; one spare. Verify battery bay and rules.','https://www.horizonhobby.com/product/11.1v-2200mah-3s-35c-g-tech-bashing-lipo-battery-xt60/GEA223S35X6GT.html','U.S. listing; recheck stock and price.'],
 ['C10','Gens ace IMARS D300 / GEA300WD300UW',null,1,null,'Shared U.S. AC charger. 3S balance charging; verify cables. Unit price needs quote.','https://www.horizonhobby.com/product/imars-d300-g-tech-ac-dc-300w-700w-charger-white/GEA300WD300UW.html','Price not available in retrieved page.']
];
base(options,16,[8,37,14,13,16,60,65,44]);
options.getRange('A2').values=[['Proposed purchases — USD, checked October 10, 2026']];
options.getRange('A3').values=[['Priced components only. Excludes charger quote, PCB fabrication, kicker, wiring, connectors, passives, hardware, wheels, tax and shipping. Fit/rules pending.']];
options.getRange('A4').values=[['Priced components subtotal (USD):']];
options.getRange('C4').formulas=[['=IF(COUNT(D7:D16)=0,"Not selected",SUM(E7:E16))']];
options.getRange('C4').setNumberFormat('$0.00');
options.getRange('A6:H6').values=[['ID','Candidate / SKU','Unit USD','Qty to buy','Extended USD','Compatibility condition','Purchase link / price source','Availability note']];
options.getRange('A7:H16').values=candidates;
options.getRange('A7:H16').format.wrapText=true;
options.getRange('A7:H16').format.rowHeight=62;
options.getRange('C7:C16').setNumberFormat('$0.00');
options.getRange('D7:D16').setNumberFormat('0');
options.getRange('E7:E16').setNumberFormat('$0.00');
options.getRange('D7:D16').format.fill='#FFF1CF';
options.dataValidations.add({range:'D7:D16',rule:{type:'whole',operator:'between',formula1:0,formula2:10000}});
options.getRange('E7:E16').formulas=candidates.map((_,i)=>[`=IF(ISNUMBER(D${i+7}),IF(ISNUMBER(C${i+7}),C${i+7}*D${i+7},"Quote needed"),"")`]);
options.getRange('G7:G16').format.font={name:'Arial',size:11,color:'#1B5EAA',underline:'single'};
options.tables.add('A6:H16',true,'ProposedPurchases');
// Exercise the quantity calculation without leaving a fabricated order in the file.
options.getRange('D7').values=[[2]];
wb.recalculate();
console.log('TEST qty=2',JSON.stringify(options.getRange('C4:E7').values));
options.getRange('D7').values=[[0]];
wb.recalculate();
console.log('TEST qty=0',JSON.stringify(options.getRange('C4:E7').values));
options.getRange('D7').clear({applyTo:'contents'});
wb.recalculate();
console.log('TEST blank',JSON.stringify(options.getRange('E7').values));
options.getRange('D7').values=[[1]];
wb.recalculate();
console.log((await wb.inspect({kind:'match',searchTerm:'#REF!|#DIV/0!|#VALUE!|#NAME\\?|#N/A|#NUM!|#NULL!',options:{useRegex:true,maxResults:50}})).ndjson);
console.log((await wb.inspect({kind:'table',range:'Proposed purchases!C4:E16',include:'values,formulas',tableMaxRows:13,tableMaxCols:3})).ndjson);
for(const [s,r,n] of [[parts,'A1:H18','parts-top'],[parts,'A19:H32','parts-bottom'],[options,'A1:H16','options']]) {
 const p=await wb.render({sheetName:s.name,range:r,scale:1,format:'png'});
 await fs.writeFile(`${out}inspection/${n}.png`,new Uint8Array(await p.arrayBuffer()));
}
await (await SpreadsheetFile.exportXlsx(wb)).save(`${out}thebus-preliminary-parts.xlsx`);
