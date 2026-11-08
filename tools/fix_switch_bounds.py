#!/usr/bin/env python3
"""Extend config functions whose branches or switch tables leave their bounds.

Usage: fix_switch_bounds.py <generated/ppc> <config.toml>  (then recompile; repeat until
the generated code has no "// ERROR" lines).

The analyser sometimes splits one function in two (for example a leaf function whose
loop sits after an unconditional branch). Branches across the split come out as
"// ERROR <target>" in the generated code. For each one, the function is widened to
run from the start of whichever piece comes first to the end of whichever comes last.
"""
import re,sys,glob,bisect
gen=sys.argv[1]; cfg=sys.argv[2]
mp=sorted({int(m,16) for m in re.findall(r'\{ 0x([0-9A-F]+), sub_', open(gen+'/ppc_func_mapping.cpp').read())})
def start_of(a):
    i=bisect.bisect_right(mp,a)-1
    return mp[max(i,0)]
def end_of(a):
    j=bisect.bisect_right(mp,a)
    return mp[j] if j<len(mp) else a+4
need={}   # function start -> end
for f in glob.glob(gen+'/ppc_recomp.*.cpp'):
    cur=None
    for line in open(f):
        m=re.match(r'PPC_FUNC_IMPL\(__imp__sub_([0-9A-F]+)\)',line)
        if m: cur=int(m[1],16); continue
        m=re.search(r'// ERROR:? (?:0x)?([0-9A-F]{8})',line)
        if m and cur is not None:
            t=int(m[1],16)
            lo=start_of(min(cur,t)); hi=end_of(max(cur,t))
            if lo in need: hi=max(hi,need[lo])
            need[lo]=hi
s=open(cfg).read()
MARK="    # Function starts inside the ranges above"
head,sep,inner=s.partition(MARK)
inner_block=(sep+inner.split("\n]",1)[0]) if sep else ""
existing={int(a,16):int(b,16) for a,b in re.findall(r'\{ address = 0x([0-9A-F]+), size = 0x([0-9A-F]+) \}',head)}
added=0
for lo,hi in need.items():
    # A widened range that now covers other entries absorbs them.
    for a in [a for a in existing if lo < a < hi]:
        hi=max(hi,a+existing.pop(a))
    if existing.get(lo,0)<hi-lo:
        existing[lo]=hi-lo; added+=1
body="\n".join(f"    {{ address = 0x{a:08X}, size = 0x{b:X} }}," for a,b in sorted(existing.items()))
# Inner starts (real functions inside a widened range, kept callable) stay as they are;
# regenerate them from a run without the widened ranges if the ranges change.
s=re.sub(r'functions = \[\n.*?\n\]', lambda m: 'functions = [\n'+body+'\n'+inner_block.rstrip("\n")+'\n]', s, flags=re.S)
open(cfg,'w').write(s)
print("functions:",len(existing),"added/changed:",added)
