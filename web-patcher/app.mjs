import {detectROM, validateManifest, ROM_SIZE} from './builder.mjs';

const $ = id => document.getElementById(id);
const themeKey = 'firered-evolved-theme';
function setTheme(theme) {
  document.documentElement.dataset.theme = theme;
  $('theme').textContent = theme === 'dark' ? 'Light mode' : 'Night mode';
  $('theme').setAttribute('aria-pressed', String(theme === 'dark'));
}
try {
  setTheme(localStorage.getItem(themeKey) === 'light' ? 'light' : 'dark');
} catch {
  setTheme('dark');
}
$('theme').addEventListener('click', () => {
  const theme = document.documentElement.dataset.theme === 'dark' ? 'light' : 'dark';
  setTheme(theme);
  try { localStorage.setItem(themeKey, theme); } catch {}
});

let manifest, selected, busy = false, checking = false, generation = 0;
function status(message, state = '') {
  $('status').textContent = message;
  $('status').dataset.state = state;
}
function refresh() {
  $('apply').disabled = busy || checking || !selected;
  $('rom').disabled = busy || !manifest;
}
async function checkROM(file) {
  if (!manifest || busy) return;
  const check = ++generation;
  selected = null;
  checking = Boolean(file);
  $('selected').textContent = 'Build your 3DS game';
  refresh();
  if (!file) { status(''); return; }
  status('Checking your ROM…');
  try {
    if (file.size !== ROM_SIZE) throw new Error('Select a clean 16 MiB US/English FireRed ROM.');
    const bytes = await file.arrayBuffer();
    const index = await detectROM(bytes, manifest);
    if (check !== generation) return;
    selected = bytes;
    $('selected').textContent = 'Detected FireRed ' + manifest.sources[index].revision;
    status('ROM verified. Ready to build.', 'success');
  } catch (error) {
    if (check === generation) status(error.message, 'error');
  } finally {
    if (check === generation) {
      checking = false;
      refresh();
    }
  }
}

$('rom').addEventListener('change', () => checkROM($('rom').files[0]));
$('drop-zone').addEventListener('dragover', event => {
  event.preventDefault();
  if (!busy) $('drop-zone').dataset.dragging = 'true';
});
$('drop-zone').addEventListener('dragleave', () => delete $('drop-zone').dataset.dragging);
$('drop-zone').addEventListener('drop', event => {
  event.preventDefault();
  delete $('drop-zone').dataset.dragging;
  if (!busy && manifest && event.dataTransfer.files.length === 1) {
    $('rom').files = event.dataTransfer.files;
    checkROM(event.dataTransfer.files[0]);
  }
});

$('apply').addEventListener('click', () => {
  if (!selected || busy || checking) return;
  busy = true;
  refresh();
  status('Starting local build…');
  let worker;
  const finish = () => {
    worker?.terminate();
    busy = false;
    refresh();
  };
  try {
    worker = new Worker(new URL('./worker.js', import.meta.url));
    worker.onerror = () => {
      finish();
      status('The browser could not run the local builder.', 'error');
    };
    worker.onmessage = ({data}) => {
      if (data.progress) { status(data.progress); return; }
      finish();
      if (data.error) { status(data.error, 'error'); return; }
      const url = URL.createObjectURL(new Blob([data.output], {type:'application/octet-stream'}));
      const link = document.createElement('a');
      link.href = url;
      link.download = manifest.distribution_scope === 'public-patch-data'
        ? `FireRed_Evolv3D_${manifest.version}.3dsx` : 'FireRed-Evolv3D-voxel.3dsx';
      document.body.append(link);
      link.click();
      link.remove();
      setTimeout(() => URL.revokeObjectURL(url), 60000);
      status('Build verified. Your 3DSX download is ready.', 'success');
    };
    // Keep the selected source so a failed build can be retried.
    const copy = selected.slice(0);
    worker.postMessage({rom: copy, manifest}, [copy]);
  } catch (error) {
    finish();
    status(error.message, 'error');
  }
});

try {
  const response = await fetch('release.json', {cache:'no-store'});
  if (!response.ok) throw new Error('Local build files are unavailable.');
  manifest = validateManifest(await response.json());
  $('version').textContent = manifest.version + ' · Nintendo 3DS';
  refresh();
} catch (error) {
  $('version').textContent = 'Build unavailable';
  status(error.message, 'error');
}
