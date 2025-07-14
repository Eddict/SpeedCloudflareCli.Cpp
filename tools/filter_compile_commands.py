import json
import sys
import os

if len(sys.argv) != 4:
    print(f"Usage: {sys.argv[0]} <src_dir> <infile> <outfile>")
    sys.exit(1)

src_dir = os.path.abspath(sys.argv[1])
infile = sys.argv[2]
outfile = sys.argv[3]

with open(infile) as f:
    db = json.load(f)

filtered = [entry for entry in db if os.path.abspath(entry['file']).startswith(src_dir)]

with open(outfile, 'w') as f:
    json.dump(filtered, f, indent=2)
