"""Import Freerouting's SES wire/via shapes when KiCad's SWIG importer returns false."""
import sys,re,os
from pathlib import Path
import pcbnew as p
root=Path(sys.argv[1]).resolve();board_name=root.name
tokens=re.findall(r'"(?:\\.|[^"\\])*"|[^\s()]+|[()]',(root/(board_name+'.ses')).read_text())
def parse(i):
 assert tokens[i]=='('; out=[];i+=1
 while tokens[i]!=')':
  if tokens[i]=='(':child,i=parse(i);out.append(child)
  else:out.append(tokens[i]);i+=1
 return out,i+1
tree,_=parse(0)
def children(node,tag):return [x for x in node if isinstance(x,list) and x and x[0]==tag]
def one(node,tag):return children(node,tag)[0]
network=one(one(tree,'routes'),'network_out')
board=p.LoadBoard(str(root/(board_name+'.kicad_pcb')))
nets=board.GetNetsByName();scale=10000
layers={'F.Cu':p.F_Cu,'In1.Cu':p.In1_Cu,'In2.Cu':p.In2_Cu,'B.Cu':p.B_Cu}
def pos(x,y):return p.VECTOR2I(p.FromMM(int(x)/scale),p.FromMM(-int(y)/scale))
counts={'nets':0,'tracks':0,'vias':0}
for net in children(network,'net'):
 net_name=net[1][1:-1] if net[1].startswith('"') else net[1]
 assert net_name in nets,net_name
 n=nets[net_name];counts['nets']+=1
 for wire in children(net,'wire'):
  path=one(wire,'path');layer=layers[path[1]];width=p.FromMM(int(path[2])/scale)
  coords=[pos(path[j],path[j+1]) for j in range(3,len(path),2)]
  for a,b in zip(coords,coords[1:]):
   if a==b:continue
   track=p.PCB_TRACK(board);track.SetStart(a);track.SetEnd(b);track.SetWidth(width);track.SetLayer(layer);track.SetNet(n);board.Add(track);counts['tracks']+=1
 for item in children(net,'via'):
  d=re.search(r'_(\d+):(\d+)_um',item[1]);assert d,item
  via=p.PCB_VIA(board);via.SetPosition(pos(item[2],item[3]));via.SetWidth(p.FromMM(int(d[1])/1000));via.SetDrill(p.FromMM(int(d[2])/1000));via.SetLayerPair(p.F_Cu,p.B_Cu);via.SetNet(n);board.Add(via);counts['vias']+=1
board.GetDesignSettings().m_MinTrackWidth=p.FromMM(.15)
assert len(list(board.GetTracks()))==counts['tracks']+counts['vias']
tmp=root/(board_name+'.routed.tmp.kicad_pcb')
assert p.SaveBoard(str(tmp),board)
os.replace(tmp,root/(board_name+'.kicad_pcb'))
print(counts)
