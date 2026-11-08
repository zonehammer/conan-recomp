#!/usr/bin/env python3
"""Extend config functions whose switch tables jump past their end.

Usage: fix_switch_bounds.py <generated/ppc> <config.toml>  (then recompile; repeat until
the recompiler reports no "Switch case ... outside function" errors).
"""
import re,sys,glob,bisect
gen=sys.argv[1]; cfg=sys.argv[2]
mp=sorted({int(m,16) for m in re.findall(r'\{ 0x([0-9A-F]+), sub_', open(gen+'/ppc_func_mapping.cpp').read())})
need={}
for f in glob.glob(gen+'/ppc_recomp.*.cpp'):
    cur=None
    for line in open(f):
        m=re.match(r'PPC_FUNC_IMPL\(__imp__sub_([0-9A-F]+)\)',line)
        if m: cur=int(m[1],16); continue
        m=re.search(r'// ERROR: 0x([0-9A-F]+)',line)
        if m and cur is not None:
            t=int(m[1],16)
            need[cur]=max(need.get(cur,0),t)
s=open(cfg).read()
existing={int(a,16):int(b,16) for a,b in re.findall(r'\{ address = 0x([0-9A-F]+), size = 0x([0-9A-F]+) \}',s)}
added=0
for f,t in need.items():
    if t<f: 
        print("target before function",hex(f),hex(t)); continue
    j=bisect.bisect_right(mp,t)
    end=mp[j] if j<len(mp) else t+4
    size=end-f
    if existing.get(f,0)<size:
        existing[f]=size; added+=1
body="\n".join(f"    {{ address = 0x{a:08X}, size = 0x{b:X} }}," for a,b in sorted(existing.items()))
s=re.sub(r'functions = \[\n.*?\n\]', 'functions = [\n'+body+'\n]', s, flags=re.S)
open(cfg,'w').write(s)
print("functions:",len(existing),"added/changed:",added)
