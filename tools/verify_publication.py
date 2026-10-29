"""Verify the source preparation; optionally require local Release assets."""
from pathlib import Path
import argparse
import hashlib
import json
import re
import sys
import zipfile

ROOT = Path(__file__).resolve().parents[1]
TEXT = {'.md', '.txt', '.json', '.csv', '.tsv', '.c', '.h', '.cpp', '.hpp', '.py', '.ps1', '.cmd', '.cmake', '.yml'}
PRIVATE_IP = re.compile(r'\b192\.168\.\d{1,3}\.\d{1,3}\b')
USER_PATH = re.compile(r'[A-Za-z]:[/\\]+Users[/\\]+[^/\\\s]+', re.I)


def digest(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--require-release-assets', action='store_true')
    args = parser.parse_args()
    required = ['README.md', 'LICENSING.md', 'COPYING-GPL-3.0.txt',
                'VitaStation/CMakeLists.txt', 'VitaStation/src/main.c',
                'VitaStation/src/psp_bubbles.cpp', 'VitaStation/src/psp_media.c',
                'Caratulas-de-loja/CMakeLists.txt', 'Caratulas-de-loja/src/main.c',
                'docs/PLUGINS.md', 'docs/FALHAS_E_LIMITACOES.md',
                'docs/COMPILAR.md', 'docs/PUBLICACAO_GITHUB.md', 'docs/TESTES.md']
    for name in required:
        assert (ROOT / name).is_file(), 'Missing: ' + name
    count = 0
    for path in ROOT.rglob('*'):
        if not path.is_file():
            continue
        rel = path.relative_to(ROOT)
        if '.git' in rel.parts or '__pycache__' in rel.parts:
            continue
        if rel.parts[0] == 'release-assets':
            continue
        assert not {'console-backup', 'deployment'} & set(rel.parts), 'Private directory: ' + str(rel)
        assert path.suffix.lower() not in {'.db', '.rif', '.bak', '.part', '.pyc'}, 'Private/generated data: ' + str(rel)
        assert not path.name.startswith('TRANSFERENCIA'), 'Private transfer receipt: ' + str(rel)
        assert path.stat().st_size <= 100 * 1024 * 1024, 'Large Git object: ' + str(rel)
        if path.suffix.lower() in TEXT:
            value = path.read_text(encoding='utf-8-sig', errors='replace')
            assert not PRIVATE_IP.search(value), 'Private-network address to review: ' + str(rel)
            assert not USER_PATH.search(value), 'Personal Windows path: ' + str(rel)
        count += 1
    readme = (ROOT / 'README.md').read_text(encoding='utf-8')
    for target in re.findall(r'\]\(([^)]+)\)', readme):
        if '://' not in target and not target.startswith('#'):
            assert (ROOT / target.split('#')[0]).exists(), 'Broken README link: ' + target
    manifest = ROOT / 'MANIFESTO_PUBLICACAO.json'
    if manifest.exists():
        for row in json.loads(manifest.read_text(encoding='utf-8'))['files']:
            path = ROOT / row['path']
            assert path.is_file() and path.stat().st_size == row['bytes'], row['path']
            assert digest(path) == row['sha256'], 'Source changed: ' + row['path']
    assets = ROOT / 'release-assets'
    filenames = ['VitaStation-1.6.6-beta.vpk', 'VitaStation-1.6.6-public-source.zip']
    present = all((assets / name).is_file() for name in filenames)
    if args.require_release_assets:
        assert present, 'Missing Release assets'
    if present:
        sums = assets / 'SHA256SUMS.txt'
        assert sums.is_file(), 'Missing checksums'
        for line in sums.read_text(encoding='ascii').splitlines():
            expected, name = line.split('  ', 1)
            assert digest(assets / name) == expected, 'Release checksum: ' + name
        for name in filenames:
            with zipfile.ZipFile(assets / name) as archive:
                assert archive.testzip() is None, 'ZIP CRC failure: ' + name
        print('Release assets: SHA-256 and ZIP CRC verified.')
    else:
        print('Source-only checkout: Release binaries not present locally.')
    print(f'Publication verified: {count} source/documentation/resource files; README links OK.')


if __name__ == '__main__':
    try:
        main()
    except (AssertionError, OSError, ValueError, zipfile.BadZipFile) as error:
        print('FAILED:', error, file=sys.stderr)
        raise SystemExit(1)
