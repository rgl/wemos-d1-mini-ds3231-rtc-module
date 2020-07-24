#!/bin/bash
set -euxo pipefail
npx preact build --no-sw --no-esm --no-prerender
sed -i 's,<link rel="manifest" href="/manifest.json">,,g' build/index.html
gzip -9 build/index.html
rm -rf ../data && mkdir ../data
(python3 | xargs -I% cp % ../data) <<'EOF'
import os
import re
include_filter = re.compile(r'^.+\.(html|css|js)(\.gz)?$')
exclude_filter = re.compile(r'^sw-debug\.js(\.gz)?$')
files = set('build/'+p for p in os.listdir('build') if include_filter.match(p) and not exclude_filter.match(p))
for p in set(p for p in files if p.endswith('.gz')):
    files.discard(p[:-3])
for p in files:
    print(p)
EOF
cd ..
platformio run --target buildfs
cat <<EOF

Upload the built filesystem to the device with:

    (cd .. && source ./venv/bin/activate && platformio run --target uploadfs)

EOF
