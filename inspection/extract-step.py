import sys, json, math
from pathlib import Path
sys.path.insert(0,'/private/tmp/thebus-cad-libs')
from OCP.STEPControl import STEPControl_Reader
from OCP.IFSelect import IFSelect_RetDone
from OCP.TopExp import TopExp_Explorer
from OCP.TopAbs import TopAbs_FACE, TopAbs_WIRE, TopAbs_REVERSED
from OCP.TopoDS import TopoDS
from OCP.BRepAdaptor import BRepAdaptor_Surface, BRepAdaptor_Curve
from OCP.BRepTools import BRepTools, BRepTools_WireExplorer
from OCP.BRepGProp import BRepGProp
from OCP.GProp import GProp_GProps
from OCP.BRepBndLib import BRepBndLib
from OCP.Bnd import Bnd_Box
from OCP.GeomAbs import GeomAbs_Plane, GeomAbs_Line, GeomAbs_Circle
base=Path(__file__).resolve().parent.parent
r=STEPControl_Reader()
assert r.ReadFile(str(base/'cad/Line PCB Outline - COMPOUND.step'))==IFSelect_RetDone
r.TransferRoots(); shape=r.OneShape()
box=Bnd_Box(); BRepBndLib.Add_s(shape,box)
bounds=[getattr(box.CornerMin(),k)() for k in ('X','Y','Z')]+[getattr(box.CornerMax(),k)() for k in ('X','Y','Z')]
print('bbox mm',bounds)
faces=[]; e=TopExp_Explorer(shape,TopAbs_FACE)
while e.More():
 f=TopoDS.Face(e.Current()); a=BRepAdaptor_Surface(f)
 if a.GetType()==GeomAbs_Plane:
  n=a.Plane().Axis().Direction(); g=GProp_GProps(); BRepGProp.SurfaceProperties_s(f,g)
  if abs(n.Z())>.999999: faces.append((g.Mass(),a.Plane().Location().Z(),f))
 e.Next()
print('horizontal faces',[(round(a,3),round(z,6)) for a,z,f in sorted(faces,key=lambda x:-x[0])[:12]])
area,z,face=max(faces,key=lambda x:x[0]); outer=BRepTools.OuterWire_s(face)
loops=[]; ex=TopExp_Explorer(face,TopAbs_WIRE)
while ex.More():
 w=TopoDS.Wire(ex.Current()); we=BRepTools_WireExplorer(w,face); segs=[]
 while we.More():
  edge=we.Current(); c=BRepAdaptor_Curve(edge); u,v=c.FirstParameter(),c.LastParameter()
  if edge.Orientation()==TopAbs_REVERSED:u,v=v,u
  def point(t):
   p=c.Value(t); return [p.X(),p.Y(),p.Z()]
  seg={'start':point(u),'end':point(v)}
  if c.GetType()==GeomAbs_Line:seg['kind']='line'
  elif c.GetType()==GeomAbs_Circle:
   circle=c.Circle(); cent=circle.Location()
   seg.update(kind='arc',mid=point((u+v)/2),center=[cent.X(),cent.Y(),cent.Z()],radius=circle.Radius(),sweep=abs(v-u))
  else:
   seg.update(kind='polyline',points=[point(u+(v-u)*i/100) for i in range(101)])
  segs.append(seg); we.Next()
 loops.append({'outer':w.IsSame(outer),'segments':segs});ex.Next()
(base/'cad/line-outline.json').write_text(json.dumps({'bbox':bounds,'face_z':z,'face_area':area,'units':'mm','loops':loops},indent=2))
print('loops',[(l['outer'],len(l['segments'])) for l in loops])
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
fig,ax=plt.subplots(figsize=(8,8))
for l in loops:
 for s in l['segments']:
  if s['kind']=='arc':
   cx,cy,_=s['center']; p=s['start']; q=s['mid']; end=s['end']; a=math.atan2(p[1]-cy,p[0]-cx); m=math.atan2(q[1]-cy,q[0]-cx); b=math.atan2(end[1]-cy,end[0]-cx); span=(b-a)%(2*math.pi)
   if (m-a)%(2*math.pi)>span+1e-8:span-=2*math.pi
   if abs(span)<1e-8:span=2*math.pi
   pts=[[cx+s['radius']*math.cos(a+span*t/100),cy+s['radius']*math.sin(a+span*t/100)] for t in range(101)]
  elif s['kind']=='polyline':pts=s['points']
  else:pts=[s['start'],s['end']]
  ax.plot([p[0] for p in pts],[p[1] for p in pts],color='#253D5A',lw=.7)
ax.set_aspect('equal');ax.set_xlabel('X (mm)');ax.set_ylabel('Y (mm)');ax.set_title('Line PCB geometry extracted from supplied STEP');ax.grid(alpha=.2)
fig.savefig(base/'inspection/line-step-outline.png',dpi=140)
