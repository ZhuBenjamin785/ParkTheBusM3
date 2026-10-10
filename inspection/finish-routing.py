"""Final layout corrections after Specctra route import; run with KiCad Python."""
import sys, json, re
from pathlib import Path
import wx
_app = wx.App(False)
import pcbnew as p

root = Path(sys.argv[1]).resolve()
name = root.name
path = root / (name + '.kicad_pcb')
board = p.LoadBoard(str(path))
footprints = {part['ref']: part['footprint'] for part in json.loads((root / 'design.json').read_text())['parts']}
for fp in board.GetFootprints():
    ref = fp.GetReference()
    if ref in footprints:
        fp.SetFPIDAsString(footprints[ref])
    if ref.startswith('H'):
        fp.SetAttributes(fp.GetAttributes() | p.FP_BOARD_ONLY)
    if (name == 'line-ring' and ref == 'J1') or (name == 'main-control' and ref in ('J3', 'J19')):
        fp.Reference().SetVisible(False)
    if name == 'main-control' and ref == 'J5':
        for shape in fp.GraphicalItems():
            if shape.GetLayer() == p.F_SilkS:
                shape.SetLayer(p.F_Fab)
        custom_name = 'JST_XH_B6B-XH-A_1x06_P2.50mm_Vertical_NoEdgeSilk'
        fp.SetFPIDAsString('THEBUS:' + custom_name)
        custom_lib = root / 'THEBUS.pretty'
        custom_lib.mkdir(exist_ok=True)
        p.PCB_IO_KICAD_SEXPR().FootprintSave(str(custom_lib), fp)
        (root / 'fp-lib-table').write_text('(fp_lib_table (lib (name "THEBUS") (type "KiCad") (uri "${KIPRJMOD}/THEBUS.pretty") (options "") (descr "Robot-specific connector footprint")))\n')
        sch_path = root / (name + '.kicad_sch')
        sch = sch_path.read_text()
        pattern = r'(\(symbol \(lib_id "THEBUS:J5"\).*?\(property "Footprint" )"Connector_JST:JST_XH_B6B-XH-A_1x06_P2.50mm_Vertical"'
        sch, count = re.subn(pattern, r'\1"THEBUS:' + custom_name + '"', sch, count=1, flags=re.S)
        assert count == 1
        sch_path.write_text(sch)
    if name == 'main-control' and ref == 'U3':
        for pad in fp.Pads():
            number = int(pad.GetNumber())
            if number in list(range(11,17)):
                channel = number-1 if number < 10 else 19-number
                side = 'A' if number < 10 else 'B'
                net_name = f'unconnected-(U3-{side}{channel}-Pad{number})'
                if net_name not in board.GetNetsByName():
                    net = p.NETINFO_ITEM(board,net_name,len(board.GetNetsByName())+1)
                    board.Add(net)
                pad.SetNet(board.GetNetsByName()[net_name])
for track in board.GetTracks():
    if not isinstance(track, p.PCB_VIA) and track.GetWidth() < p.FromMM(.15):
        track.SetWidth(p.FromMM(.15))
p.SaveBoard(str(path), board)
