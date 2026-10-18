"""Reject game/save/build payloads in every reachable public push commit."""
import argparse,json,re,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser();p.add_argument('--refs',nargs='+');p.add_argument('--pre-push',nargs=2,metavar=('REMOTE','URL'));a=p.parse_args()
refs=a.refs or []
if a.pre_push:
 url=a.pre_push[1].lower().rstrip('/').removesuffix('.git')
 if url not in ['https://github.com/niceliketryce/firered-evolved-3ds','git@github.com:niceliketryce/firered-evolved-3ds']:
  raise SystemExit('Push blocked: this checkout publishes only to the owner\'s3DS fork.')
 try:
  destination=json.loads(subprocess.check_output(['gh','api','repos/NiceLikeTryce/firered-evolved-3ds'],text=True))
 except (OSError,subprocess.CalledProcessError,ValueError) as error:
  raise SystemExit('Push blocked: cannot verify private3DS destination.') from error
 if destination.get('id')!=1424368097 or destination.get('private') is not True or destination.get('visibility')!='private':
  raise SystemExit('Push blocked: the3DS project must remain PRIVATE.')
 for line in sys.stdin:
  local,sha,remote,old=line.split()
  if sha!='0'*40:refs.append(sha)
if not refs:refs=['HEAD']
text=subprocess.check_output(['git','rev-list','--objects',*refs],cwd=ROOT,text=True)
blocked=[]
for line in text.splitlines():
 parts=line.split(' ',1)
 if len(parts)<2:continue
 path=parts[1]
 if re.search(r'\.(?:gba|agb|nds|3ds|cia|sav|srm|fla|3dsx|smdh|pak|recipe|elf|o|bps|ips|ups)$',path,re.I):blocked.append(path)
if blocked:raise SystemExit('Public push blocked by payload paths:\n'+'\n'.join(sorted(set(blocked))))
print('PASS public3DS fork whole-history game/save/build payload guard')
