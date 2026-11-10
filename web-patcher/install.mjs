import {sha256, validateManifest} from './builder.mjs';

const $=id=>document.getElementById(id);
const base='/FireRed-Evolv3D/';
let theme='dark';try{const value=localStorage.getItem('firered-evolved-theme');if(['light','dark'].includes(value))theme=value;}catch{}
function paint(){document.documentElement.dataset.theme=theme;$('theme').textContent=theme==='dark'?'Light mode':'Dark mode';}
paint();$('theme').addEventListener('click',()=>{theme=theme==='dark'?'light':'dark';paint();try{localStorage.setItem('firered-evolved-theme',theme);}catch{}});

let release, knownBuilds=[], checkGeneration=0;
const installed=$('installed-game');
const versionStatus=$('installed-version');
installed.addEventListener('change',async()=>{
  const generation=++checkGeneration;
  const file=installed.files[0];
  if(!file||!release){
    versionStatus.textContent='Choose the .3dsx from your SD card to check its version.';
    return;
  }
  versionStatus.textContent='Checking the installed game…';
  try{
    const candidates=knownBuilds.filter(build=>build.output.size===file.size);
    const hash=candidates.length ? await sha256(await file.arrayBuffer()) : null;
    if(generation!==checkGeneration)return;
    const match=candidates.find(build=>build.output.sha256===hash);
    versionStatus.textContent=match===release
      ? 'Verified installed version: '+match.version+'. It matches the version available here.'
      : match
        ? 'Verified installed version: '+match.version+'. The patcher offers '+release.version+'.'
        : 'Installed version could not be verified against the builds listed by this patcher.';
  }catch{
    if(generation===checkGeneration)versionStatus.textContent='Could not read the selected .3dsx. Try copying it to your computer first.';
  }
});

async function previousBuilds(manifest){
  const source=manifest.downloads?.service;
  if(source?.asset!=='service.json'||!Number.isSafeInteger(source.size)||source.size<1||
     source.size>16*1024||!/^[a-f0-9]{64}$/.test(source.sha256))return [];
  try{
    const response=await fetch(base+'service.json',{cache:'no-store'});
    if(!response.ok)return [];
    const bytes=await response.arrayBuffer();
    if(bytes.byteLength!==source.size||await sha256(bytes)!==source.sha256)return [];
    const service=JSON.parse(new TextDecoder().decode(bytes));
    if(service.schema!==1||service.application!=='FireRed-Evolv3D'||service.version!==manifest.version||
       service.output?.sha256!==manifest.output.sha256||service.output?.size!==manifest.output.size||
       !Array.isArray(service.patches)||service.patches.length>8)return [];
    return service.patches.filter(entry=>entry&&/^v\d+\.\d+\.\d+$/.test(entry.from_version)&&
      Number.isSafeInteger(entry.source_size)&&entry.source_size>=44&&entry.source_size<=192*1024*1024&&
      /^[a-f0-9]{64}$/.test(entry.source_sha256)).map(entry=>({
        version:entry.from_version,output:{size:entry.source_size,sha256:entry.source_sha256}
      }));
  }catch{return [];}
}

function descriptor(downloads,key,name,limit){
  const item=downloads?.[key];if(!item||item.asset!==name||!Number.isSafeInteger(item.size)||item.size<1||item.size>limit||!/^[a-f0-9]{64}$/.test(item.sha256))throw Error('The current installation tools are not available yet. You can still use the builder and copy its game to your SD card.');
  return base+'downloads/'+item.asset;
}
try{
  const response=await fetch(base+'release.json',{cache:'no-store'});if(!response.ok)throw Error('Current installation information is unavailable.');
  const manifest=validateManifest(await response.json());
  if(manifest.distribution_scope!=='public-patch-data')throw Error('Public build information is unavailable.');
  release=manifest;
  $('available-build').textContent=manifest.version;
  knownBuilds=[manifest,...(manifest.base_build?[manifest.base_build]:[]),...await previousBuilds(manifest)];
  installed.disabled=false;
  if(manifest.launcher_abi!==1)throw Error('Use the current builder to prepare a launcher-compatible game.');
  const cia=descriptor(manifest.downloads,'launcher','FireRed-Evolv3D-Launcher.cia',4*1024*1024);
  const qr=descriptor(manifest.downloads,'launcher_qr','FireRed-Evolv3D-Launcher-QR.png',128*1024);
  const wireless=descriptor(manifest.downloads,'wireless','FireRed-Evolv3D-Wireless.zip',64*1024*1024);
  $('launcher-download').href=cia;$('launcher-download').hidden=false;$('launcher-qr').src=qr;$('launcher-qr').hidden=false;
  $('wireless-download').href=wireless;$('wireless-download').hidden=false;$('tools-status').textContent='Installation tools for '+manifest.version+'.';
}catch(error){
  $('tools-status').textContent=error.message;
  if(!release)$('available-build').textContent='Unavailable';
}
