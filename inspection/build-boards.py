"""Deterministic KiCad sources. Run with KiCad's embedded Python 3.9."""
import wx
_kicad_app = wx.App(False)
import pcbnew as p, math, json, uuid, csv
from pathlib import Path
B=Path(__file__).resolve().parent.parent
LIB=Path('/Applications/KiCad/KiCad.app/Contents/SharedSupport/footprints')
def uid(s): return str(uuid.uuid5(uuid.NAMESPACE_URL,'thebus/revA/'+s))
def v(x,y):return p.VECTOR2I(p.FromMM(x),p.FromMM(y))
def quoted(s):return json.dumps(str(s))
def arc(c,r,a,b):
 return dict(kind='arc',start=[c[0]+r*math.cos(a),c[1]+r*math.sin(a)],mid=[c[0]+r*math.cos((a+b)/2),c[1]+r*math.sin((a+b)/2)],end=[c[0]+r*math.cos(b),c[1]+r*math.sin(b)],center=list(c),radius=r)
main=[]
for k in range(4):
 a=k*math.pi/2; main.append(arc((0,0),60,a-.7353773065916781,a+.7353773065916781))
 t=a+math.pi/4;c=(63*math.cos(t),63*math.sin(t));tab=arc(c,3,t-math.pi/2,t+math.pi/2)
 main.append(dict(kind='line',start=main[-1]['end'],end=tab['start']));main.append(tab)
 end=[60*math.cos(a+math.pi/2-.7353773065916781),60*math.sin(a+math.pi/2-.7353773065916781)]
 main.append(dict(kind='line',start=tab['end'],end=end))
points=[(18.5,-32),(24.547875144394,-32),(24.547875144394,-19),(40,-19)]
cut=[dict(kind='line',start=a,end=b) for a,b in zip(points,points[1:])]
cut.append(arc((7.37233632742414,2.773937572197),39.2258689403901,-.5884726944714658,.5884726944714658))
points=[(40,24.547875144394),(24.547875144394,24.547875144394),(24.547875144394,39),(18.5,39),(18.5,-32)]
cut +=[dict(kind='line',start=a,end=b) for a,b in zip(points,points[1:])]
mainloops=[dict(outer=True,segments=main),dict(outer=False,segments=cut)]
mainholes=[(x,y,3.0) for x in [-44.5477272147525,44.5477272147525] for y in [-44.5477272147525,44.5477272147525]]
raw=json.loads((B/'cad/line-outline.json').read_text())
lineloops=[l for l in raw['loops'] if l['outer'] or len(l['segments'])>1]
lineholes=[(*l['segments'][0]['center'][:2],2*l['segments'][0]['radius']) for l in raw['loops'] if len(l['segments'])==1 and l['segments'][0].get('radius',0)>=1.5]
# Close micron-sized STEP edge tolerance gaps by moving line endpoints to their neighboring arcs.
for loop in lineloops:
 seg=loop['segments']
 for i,s in enumerate(seg):
  n=seg[(i+1)%len(seg)]
  if math.dist(s['end'][:2],n['start'][:2])>.000001:
   if s['kind']=='line':s['end']=n['start']
   else:n['start']=s['end']
class Design:
 def __init__(self,name,loops,holes):
  self.name=name;self.root=uid(name);self.dir=B/'pcbs'/name;self.dir.mkdir(parents=True,exist_ok=True)
  self.board=p.BOARD();self.board.SetCopperLayerCount(4);self.board.GetDesignSettings().SetBoardThickness(p.FromMM(1.6));self.parts=[];self.nets={};self.loops=loops
  for loop in loops:
   for s in loop['segments']:
    g=p.PCB_SHAPE();g.SetLayer(p.Edge_Cuts);g.SetWidth(p.FromMM(.05))
    def pt(a):return v(100+a[0],100-a[1])
    if s['kind']=='arc':g.SetShape(p.SHAPE_T_ARC);g.SetArcGeometry(pt(s['start']),pt(s['mid']),pt(s['end']))
    else:g.SetShape(p.SHAPE_T_SEGMENT);g.SetStart(pt(s['start']));g.SetEnd(pt(s['end']))
    self.board.Add(g)
  for i,(x,y,d) in enumerate(holes):
   fp=p.FOOTPRINT(self.board);fp.SetReference('H'+str(i+1));fp.SetValue('NPTH '+str(d));fp.SetPosition(v(100+x,100-y));pad=p.PAD(fp);pad.SetAttribute(p.PAD_ATTRIB_NPTH);pad.SetShape(p.PAD_SHAPE_CIRCLE);pad.SetSize(v(d,d));pad.SetDrillSize(v(d,d));pad.SetLayerSet(p.LSET.AllCuMask());pad.SetPosition(fp.GetPosition());fp.Add(pad);self.board.Add(fp)
   fp.Reference().SetVisible(False);fp.Value().SetVisible(False);fp.SetAttributes(fp.GetAttributes() | p.FP_BOARD_ONLY)
 def net(self,name):
  if not name:return None
  if name not in self.nets:self.nets[name]=p.NETINFO_ITEM(self.board,name,len(self.nets)+1);self.board.Add(self.nets[name])
  return self.nets[name]
 def add(self,ref,value,foot,x,y,pins,angle=0,types=None,back=False):
  # pins is {physical pad number: (logical pin name, net name)}
  fp=p.FootprintLoad(str(LIB/(foot.split(':')[0]+'.pretty')),foot.split(':')[1]);assert fp,foot
  fp.SetReference(ref);fp.SetValue(value);fp.SetPosition(v(100+x,100-y));fp.SetOrientationDegrees(angle)
  self.board.Add(fp)
  fp.SetFPIDAsString(foot)
  if back: fp.Flip(fp.GetPosition(),False)
  partuid=uid(self.name+'/'+ref);path=p.KIID_PATH();path.push_back(p.KIID(self.root));path.push_back(p.KIID(partuid));fp.SetPath(path)
  for pad in fp.Pads():
   pair=pins.get(pad.GetNumber())
   if pair and pair[1]:pad.SetNet(self.net(pair[1]))
  fp.Reference().SetTextSize(v(.8,.8));fp.Reference().SetTextThickness(p.FromMM(.12));fp.Value().SetVisible(False)
  self.parts.append(dict(ref=ref,value=value,footprint=foot,pins=pins,types=types or {},uuid=partuid,x=x,y=y,angle=angle,back=back))
  return fp
 def conn(self,ref,value,x,y,nets,angle=0):
  n=len(nets);foot=f'Connector_JST:JST_XH_B{n}B-XH-A_1x{n:02}_P2.50mm_Vertical'
  return self.add(ref,value,foot,x,y,{str(i+1):(net,net) for i,net in enumerate(nets)},angle)
 def rc(self,ref,val,x,y,n1,n2,angle=0):
  cat='Resistor_SMD:R_0805_2012Metric' if ref.startswith('R') else 'Capacitor_SMD:C_0805_2012Metric'
  return self.add(ref,val,cat,x,y,{'1':('1',n1),'2':('2',n2)},angle)
 def finish(self):
  # Copper trace widths are fixed in the router rules below. Board constraints remain conservative.
  settings={'board':{'design_settings':{'rules':{'min_clearance':.15,'min_track_width':.15,'min_via_diameter':.6,'min_through_hole_diameter':.3,'min_copper_edge_clearance':.2}}},'net_settings':{'classes':[{'name':'Default','clearance':.15,'track_width':.15,'via_diameter':.7,'via_drill':.35,'diff_pair_width':.25,'diff_pair_gap':.25,'diff_pair_via_gap':.25}],'netclass_assignments':{}},'meta':{'filename':self.name+'.kicad_pro','version':1}}
  (self.dir/(self.name+'.kicad_pro')).write_text(json.dumps(settings,indent=2))
  p.SaveBoard(str(self.dir/(self.name+'.kicad_pcb')),self.board)
  assert p.ExportSpecctraDSN(self.board,str(self.dir/(self.name+'.dsn')))
  self.schematic()
  (self.dir/'design.json').write_text(json.dumps(dict(name=self.name,parts=self.parts,outline=self.loops),indent=2))
  with (self.dir/'connections.csv').open('w') as f:
   w=csv.writer(f);w.writerow(['Reference','Value','Pad','Pin','Net'])
   for part in self.parts:
    for num,(label,net) in part['pins'].items():w.writerow([part['ref'],part['value'],num,label,net or 'NC'])
 def schematic(self):
  # One self-contained sheet: explicitly named nets, pin names, physical pin numbers and NC flags.
  parts=self.parts;libs=[];instances=[];labels=[]
  for i,part in enumerate(parts):
   pins=list(part['pins'].items());N=len(pins);height=(N+1)*2.54;name=part['ref'];x=30.48+(i%5)*76.2;y=30.48+(i//5)*152.4
   lib=[f'(symbol "THEBUS:{name}" (pin_names (offset 1)) (in_bom yes) (on_board yes) (property "Reference" "{name}" (at 0 3 0) (effects (font (size 1.27 1.27)))) (property "Value" {quoted(part["value"])} (at 0 0 0) (effects (font (size 1.27 1.27))))',f'(symbol "{name}_0_1" (rectangle (start 0 0) (end 30 {-height}) (stroke (width .254) (type default)) (fill (type background))))',f'(symbol "{name}_1_1"']
   for j,(num,(label,net)) in enumerate(pins):
    py=-(j+1)*2.54;typ=part['types'].get(num,'passive')
    lib.append(f'(pin {typ} line (at -5.08 {py} 0) (length 5.08) (name {quoted(label)} (effects (font (size 1 1)))) (number {quoted(num)} (effects (font (size .9 .9)))))')
    xx=x-5.08;yy=y-py
    if net:labels.append(f'(global_label {quoted(net)} (shape input) (at {xx} {yy} 0) (effects (font (size 1 1)) (justify right bottom)) (uuid {uid(part["uuid"]+num+"label")}))')
    else:labels.append(f'(no_connect (at {xx} {yy}) (uuid {uid(part["uuid"]+num+"nc")}))')
   libs.append(''.join(lib)+'))')
   instances.append(f'(symbol (lib_id "THEBUS:{name}") (at {x} {y} 0) (unit 1) (in_bom yes) (on_board yes) (dnp no) (uuid {part["uuid"]}) (property "Reference" "{name}" (at {x+15} {y-5} 0) (effects (font (size 1.27 1.27)))) (property "Value" {quoted(part["value"])} (at {x+15} {y-2} 0) (effects (font (size 1 1)))) (property "Footprint" {quoted(part["footprint"])} (at {x} {y} 0) (effects (font (size 1 1)) (hide yes))) (instances (project {quoted(self.name)} (path "/{self.root}" (reference "{name}") (unit 1)))))')
  # Large engineering sheet retains legible labels without downscaling crowded circuitry.
  h=60+math.ceil(len(parts)/5)*152.4
  text=f'(kicad_sch (version 20250114) (generator "eeschema") (uuid {self.root}) (paper "User" 440 {h}) (lib_symbols {"".join(libs)}) {"".join(instances)} {"".join(labels)} (sheet_instances (path "/" (page "1"))))'
  (self.dir/(self.name+'.kicad_sch')).write_text(text)

m=Design('main-control',mainloops,mainholes)
# Teensy header sockets use manufacturer row sequence (top view, USB at top).
left=['GND']+[str(i) for i in range(13)]+['3V3']+[str(i) for i in range(24,33)]
right=['5V','GND','3V3']+[str(i) for i in range(23,12,-1)]+['GND']+[str(i) for i in range(41,32,-1)]
gpio={0:'CAM_TX',1:'CAM_RX',12:'AUX_PWM',13:'DRIVER_FAULT',14:'LINE_ANALOG',18:'SDA',19:'SCL',32:'REF_PLAY',33:'MCU_ENABLE',38:'BAT_SENSE',39:'LINE_CTRL',40:'AUX_A16',41:'AUX_A17'}
for i in range(5):gpio[2+i]=f'M{i+1}_PWM';gpio[7+i]=f'M{i+1}_DIR';gpio[[15,16,17,20,21][i]]=f'M{i+1}_CS';gpio[22+2*i]=f'M{i+1}_EA';gpio[23+2*i]=f'M{i+1}_EB'
for i in range(4):gpio[34+i]=f'LINE_S{i}'
for ref,row,x in [('J1',left,-24),('J2',right,-8.76)]:
 m.add(ref,'Teensy 4.1 '+('left' if ref=='J1' else 'right')+' socket','Connector_PinSocket_2.54mm:PinSocket_1x24_P2.54mm_Vertical',x,29.21,{str(i+1):(label,gpio.get(int(label)) if label.isdigit() else label) for i,label in enumerate(row)})
# Five motor driver logic plugs; motor power uses the separate fused harness.
for i,(x,y,ang) in enumerate([(-44,30,0),(-48,-20,0),(-49,-31,0),(1,-33,0),(3,30,0)]):
 m.conn(f'J{3+i}',f'M{i+1} G2 LOGIC',x,y,['GND',f'M{i+1}_PWM',f'M{i+1}_DIR','DRIVE_ENABLE',f'M{i+1}_CS','DRIVER_FAULT'],ang)
for i,(x,y) in enumerate([(-36,37),(-49,-10),(-27,-37),(-2,-44),(5,44)]):
 m.conn(f'J{8+i}',f'M{i+1} ENCODER 5V',x,y,['5V','GND',f'M{i+1}_EA_RAW',f'M{i+1}_EB_RAW'])
m.conn('J13','5V LOGIC INPUT',-15,46,['5V','GND'])
m.conn('J14','OPENMV UART ONLY',3,18,['GND','CAM_RX','CAM_TX'])
m.conn('J15','IMU 3V3',-3,5,['3V3','GND','SDA','SCL'])
m.conn('J16','LINE HARNESS',-4,-8,['3V3','GND','LINE_ANALOG','LINE_S0','LINE_S1','LINE_S2','LINE_S3','LINE_CTRL'])
m.conn('J17','REFEREE GPIO',-34,16,['GND','REF_PLAY'])
m.conn('J18','BATTERY MONITOR',-34,-9,['VBAT_SENSE_IN','GND'])
m.conn('J19','AUX LOGIC ONLY',-20,-44,['GND','AUX_PWM','AUX_A16','AUX_A17'])
m.add('U1','SN74LVC1G08DBVR','Package_TO_SOT_SMD:SOT-23-5',-33,6,{'1':('A','REF_PLAY'),'2':('B','MCU_ENABLE'),'3':('GND','GND'),'4':('Y','DRIVE_ENABLE'),'5':('VCC','3V3')},types={'1':'input','2':'input','4':'output'})
for bank,cy in enumerate([0,10]):
 pins={'1':('DIR','3V3'),'10':('GND','GND'),'19':('OE','GND'),'20':('VCC','3V3')}
 for channel in range(8):
  index=bank*8+channel
  motor=index//2+1
  side='EA' if index%2==0 else 'EB'
  pins[str(2+channel)]=(f'A{channel+1}',f'M{motor}_{side}_RAW' if index<10 else 'GND')
  pins[str(18-channel)]=(f'B{channel+1}',f'M{motor}_{side}' if index<10 else None)
 m.add(f'U{bank+2}','SN74LVC245APWR','Package_SO:TSSOP-20_4.4x6.5mm_P0.65mm',8,cy,pins,back=True,types={str(i):'input' for i in range(2,10)})
for ref,val,x,y,n1,n2 in [('R1','10k',-37,9,'REF_PLAY','GND'),('R2','10k',-37,5,'MCU_ENABLE','GND'),('R3','1k',-37,1,'DRIVE_ENABLE','GND'),('R4','10k',-35,-3,'DRIVER_FAULT','3V3'),('R5','100k 1%',5,-17,'VBAT_SENSE_IN','BAT_SENSE'),('R6','27k 1%',5,-20,'BAT_SENSE','GND'),('C1','100n 16V',-31,2,'3V3','GND'),('C2','100n 16V',5,-23,'BAT_SENSE','GND'),('C3','10u 10V',-18,40,'5V','GND'),('C4','10u 10V',-11,37,'3V3','GND'),('C5','100n 16V',14,0,'3V3','GND'),('C6','100n 16V',14,10,'3V3','GND')]:m.rc(ref,val,x,y,n1,n2)
m.finish()
l=Design('line-ring',lineloops,lineholes)
l.add('U1','CD74HC4067M','Package_SO:SOIC-24W_7.5x15.4mm_P1.27mm',-18,0,{str(i):(label,net) for i,label,net in [(1,'COM','LINE_ANALOG'),(10,'S0','LINE_S0'),(11,'S1','LINE_S1'),(12,'GND','GND'),(13,'S3','LINE_S3'),(14,'S2','LINE_S2'),(15,'EN','GND'),(24,'VCC','3V3')]+[(9-i,'I'+str(i),'SENSE_'+str(i)) for i in range(8)]+[(23-i,'I'+str(8+i),'SENSE_'+str(8+i)) for i in range(8)]},types={'10':'input','11':'input','13':'input','14':'input','15':'input'})
l.conn('J1','MAIN HARNESS',7,9,['3V3','GND','LINE_ANALOG','LINE_S0','LINE_S1','LINE_S2','LINE_S3','LINE_CTRL'])
# Equal angular spacing; module carriers are tangent to R55 below the ring.
for i in range(16):
 a=math.radians(11.25+22.5*i);cx=55*math.cos(a);cy=55*math.sin(a)
 # Compact keyed 1 mm connector fits the 10 mm radial ring; sensor heads mount below on brackets.
 angle=math.degrees(a)-90
 l.add('J'+str(i+2),'QTRX '+str(i),'Connector_JST:JST_SH_SM04B-SRSS-TB_1x04-1MP_P1.00mm_Horizontal',cx,cy,
       {str(j+1):(net,net) for j,net in enumerate(['SENSE_'+str(i),'3V3','GND','LINE_CTRL'])},angle)
l.rc('C1','100n 16V',-11,9,'3V3','GND');l.rc('C2','10u 10V',-7,9,'3V3','GND')
for i in range(4):l.rc('R'+str(i+1),'100k',-31+4*i,-11,'LINE_S'+str(i),'GND',90)
l.rc('R5','10k',10,-16,'LINE_CTRL','3V3')
l.finish()
(B/'cad/main-outline-reconstructed.json').write_text(json.dumps(dict(units='mm',source='User supplied Onshape FeatureScript initial geometry and constraints; construction circles excluded',loops=mainloops,holes=mainholes),indent=2))
print('Created main and line KiCad projects, schematics, connection tables, and routing inputs.')
