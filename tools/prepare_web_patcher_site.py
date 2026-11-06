"""Prepare a fresh Pages artifact from source and verified release payloads."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
REPOSITORY = 'NiceLikeTryce/FireRed-Evolv3D'
VERSION = re.compile(r'v(?:0|[1-9][0-9]*)\.(?:0|[1-9][0-9]*)\.(?:0|[1-9][0-9]*)')
HASH = re.compile(r'[0-9a-f]{64}')
MAX_PART = 192 * 1024 * 1024
PARTS = {
    'launcher.gz', 'launcher-recipe.gz', 'launcher-game-recipe.gz',
    'romfs-recipe.gz', 'romfs-literals.gz', 'romfs-textures.gz',
    'FireRed_Evolv3D_assets_v2_from_BPRE_USA.bps.gz',
    'FireRed_Evolv3D_assets_v2_from_BPRE_USA_Rev1.bps.gz'
}


def command(arguments):
    return subprocess.check_output(arguments, cwd=ROOT).decode('utf-8')


def validate_manifest(manifest, allow_local=False):
    if not isinstance(manifest, dict):
        raise ValueError('Invalid release manifest.')
    published = (manifest.get('distribution_scope') == 'public-patch-data'
                 and VERSION.fullmatch(str(manifest.get('version', ''))))
    local = (allow_local and manifest.get('distribution_scope') == 'owner-local'
             and re.fullmatch(r'beta-rc[0-9]+', str(manifest.get('version', ''))))
    if not (published or local) or manifest.get('schema') not in (10, 11):
        raise ValueError('Expected a qualified public patch release.')
    parts = manifest.get('parts')
    expected_parts = PARTS | ({'runtime-update.bps.gz'} if manifest['schema'] == 11 else set())
    if not isinstance(parts, dict) or set(parts) != expected_parts:
        raise ValueError('Unexpected release asset names.')
    for descriptor in parts.values():
        if not isinstance(descriptor, dict):
            raise ValueError('Invalid release asset descriptor.')
        if any(type(descriptor.get(key)) is not int or not 1 <= descriptor[key] <= MAX_PART
               for key in ('size', 'decoded_size')):
            raise ValueError('Invalid release asset size.')
        if not HASH.fullmatch(str(descriptor.get('sha256', ''))):
            raise ValueError('Invalid release asset hash.')
    return manifest


def copy_asset(source, destination, descriptor):
    if not source.is_file() or source.is_symlink() or source.stat().st_size != descriptor['size']:
        raise ValueError(f'Asset size differs: {source.name}')
    digest = hashlib.sha256()
    written = 0
    with source.open('rb') as stream, destination.open('wb') as target:
        while block := stream.read(1024 * 1024):
            written += len(block)
            if written > descriptor['size']:
                raise ValueError(f'Asset grew during preparation: {source.name}')
            digest.update(block)
            target.write(block)
    if written != descriptor['size'] or digest.hexdigest() != descriptor['sha256']:
        raise ValueError(f'Asset hash differs: {source.name}')


def copy_source(destination):
    paths = subprocess.check_output(['git', 'ls-files', '-z', '--', 'web-patcher'], cwd=ROOT)
    for value in paths.decode('utf-8').split('\0'):
        if not value:
            continue
        source = ROOT / value
        relative = source.relative_to(ROOT / 'web-patcher')
        if source.name in ('.gitignore', '.gitattributes', 'README.md'):
            continue
        supported = relative.as_posix() == 'offline-files.json' or (source.suffix == '.png' and relative.parts[0] == 'gallery') or source.suffix in ('.html', '.css', '.mjs', '.js', '.txt', '.md', '.ld') or source.name == 'LICENSE'
        if source.is_symlink() or not supported:
            raise ValueError(f'Unexpected source file: {relative}')
        target = destination / relative
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(source, target)


def verify_browser_manifest(site, manifest_path=None):
    script = """
      const {readFileSync} = await import('node:fs');
      const {pathToFileURL} = await import('node:url');
      const {validateManifest} = await import(pathToFileURL(process.argv[1]));
      validateManifest(JSON.parse(readFileSync(process.argv[2], 'utf8')));
    """
    subprocess.run(['node', '--input-type=module', '-e', script,
                    str(site / 'builder.mjs'), str(manifest_path or site / 'release.json')], check=True)


def download_release(tag, destination):
    repository = json.loads(command(['gh', 'api', f'repos/{REPOSITORY}']))
    if repository.get('id') != 1424368097 or repository.get('private') is not False:
        raise ValueError('Unexpected public repository identity.')
    release = json.loads(command(['gh', 'api', f'repos/{REPOSITORY}/releases/tags/{tag}']))
    if (release.get('draft') is not False or not release.get('published_at')
            or release.get('author', {}).get('login') != 'NiceLikeTryce'
            or release.get('tag_name') != tag):
        raise ValueError('Expected a published owner release.')
    assets = {asset['name']: asset for asset in release['assets']}
    if 'release.json' not in assets or not 1 <= assets['release.json']['size'] <= 1024 * 1024:
        raise ValueError('Missing or oversized release manifest.')
    command(['gh', 'release', 'download', tag, '--repo', REPOSITORY,
             '--pattern', 'release.json', '--dir', str(destination)])
    manifest = validate_manifest(json.loads((destination / 'release.json').read_text(encoding='utf-8')))
    if manifest['version'] != tag:
        raise ValueError('Release version differs from its tag.')
    verify_browser_manifest(ROOT / 'web-patcher', destination / 'release.json')
    for name, descriptor in manifest['parts'].items():
        if name not in assets or assets[name]['size'] != descriptor['size']:
            raise ValueError(f'Release asset size differs: {name}')
        command(['gh', 'release', 'download', tag, '--repo', REPOSITORY,
                 '--pattern', name, '--dir', str(destination)])
    return manifest


def assemble_site(manifest, payloads, destination):
    destination.mkdir()
    copy_source(destination)
    (destination / 'assets').mkdir()
    for name, descriptor in manifest['parts'].items():
        copy_asset(payloads / name, destination / 'assets' / name, descriptor)
    (destination / 'release.json').write_text(json.dumps(manifest, indent=2) + '\n', encoding='utf-8')
    if manifest['distribution_scope'] == 'public-patch-data':
        index = destination / 'index.html'
        text = index.read_text(encoding='utf-8')
        index.write_text(text.replace('NINTENDO 3DS HOMEBREW · LOCAL PREVIEW',
                                      'NINTENDO 3DS HOMEBREW · BETA'), encoding='utf-8')
    verify_browser_manifest(destination)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    source = parser.add_mutually_exclusive_group(required=True)
    source.add_argument('--release')
    source.add_argument('--payload-directory', type=Path)
    parser.add_argument('--manifest', type=Path)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--allow-local', action='store_true')
    args = parser.parse_args()
    if args.release and (not VERSION.fullmatch(args.release) or args.allow_local):
        raise ValueError('Expected a public vMAJOR.MINOR.PATCH release.')
    if args.payload_directory and not args.manifest:
        raise ValueError('Local preparation requires an explicit manifest.')
    output = args.output.resolve()
    source_root = (ROOT / 'web-patcher').resolve()
    if output.exists() or output == source_root or source_root in output.parents:
        raise ValueError('Choose a fresh output directory outside the source tree.')
    output.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='.evolv3d-pages-', dir=output.parent) as temporary:
        temporary = Path(temporary).resolve()
        assert temporary.is_relative_to(output.parent)
        if args.release:
            payloads = temporary / 'download'
            payloads.mkdir()
            manifest = download_release(args.release, payloads)
        else:
            payloads = args.payload_directory.resolve(strict=True)
            manifest = validate_manifest(json.loads(args.manifest.read_text(encoding='utf-8')), args.allow_local)
        stage = temporary / 'site'
        assemble_site(manifest, payloads, stage)
        stage.rename(output)
    print(f'Prepared verified patcher site: {output}')


if __name__ == '__main__':
    main()
