from pathlib import Path
root=Path(__file__).resolve().parent.parent/'pcbs'
for name in ('main-control','line-ring'):
 d=root/name;s=(d/(name+'.kicad_sch')).read_text();start=s.index('(lib_symbols');level=0
 for j in range(start,len(s)):
  if s[j]=='(':level+=1
  elif s[j]==')':
   level-=1
   if not level:break
 symbols=s[start+len('(lib_symbols'):j].replace('(symbol "THEBUS:','(symbol "')
 (d/'THEBUS.kicad_sym').write_text('(kicad_symbol_lib (version 20211014) (generator "kicad_symbol_editor") '+symbols+')')
 (d/'sym-lib-table').write_text('(sym_lib_table (lib (name "THEBUS") (type "KiCad") (uri "${KIPRJMOD}/THEBUS.kicad_sym") (options "") (descr "")))')
 print(name,len(symbols))
