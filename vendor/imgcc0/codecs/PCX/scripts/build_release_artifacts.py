#!/usr/bin/env python3
from __future__ import annotations

import argparse
import gzip
import hashlib
import json
import os
import shutil
import stat
import subprocess
import tarfile
import zipfile
from dataclasses import dataclass
from datetime import datetime, timezone
from pathlib import Path
from typing import Iterable, List, Tuple

EXCLUDED_DIR_NAMES = {
    '.git',
    '.github-cache',
    '.release',
    '.cmake-stage',
    'build-cmake',
    'dist',
    '__pycache__',
    '.pytest_cache',
    '.dest',
}
EXCLUDED_FILE_NAMES = {
    'SHA256SUMS',
    'SHA512SUMS',
    'manifest.json',
    'manifest.json.sig',
    'verification-policy.json',
    'SIGNING.json',
}
EXCLUDED_SUFFIXES = {
    '.o', '.obj', '.pdb', '.ilk', '.dSYM', '.sig'
}
DOC_CANDIDATES = [
    'PUBLIC_API.md',
    'SECURITY.md',
    'THREADING.md',
    'PERF_OBSERVABILITY.md',
    'SUPPLY_CHAIN.md',
    'REPRODUCIBLE_BUILDS.md',
    'SBOM.md',
    'PROVENANCE.md',
    'CONSUMER_VERIFICATION.md',
]
SBOM_SPEC_VERSION = '1.7'
DEFAULT_BUILD_TYPE = 'https://example.com/codec/release/v1'
DEFAULT_BUILDER_ID = 'urn:codec-release-tooling:local'
DEFAULT_SOURCE_URI_SCHEME = 'local://'


@dataclass(frozen=True)
class ReleaseNames:
    source_tar: str
    source_zip: str
    binary_tar: str
    binary_zip: str
    source_sbom: str
    binary_sbom: str
    manifest: str
    sha256sums: str
    sha512sums: str
    provenance: str
    provenance_sig: str
    manifest_sig: str
    public_key: str
    signing_meta: str
    policy: str


def stable_epoch(repo_root: Path) -> int:
    env_epoch = os.environ.get('SOURCE_DATE_EPOCH')
    if env_epoch:
        return int(env_epoch)
    latest = 315532800
    for path in repo_root.rglob('*'):
        if not path.is_file():
            continue
        rel = path.relative_to(repo_root)
        if should_exclude_source(rel):
            continue
        latest = max(latest, int(path.stat().st_mtime))
    return latest


def platform_tag() -> str:
    return f"{os.uname().sysname.lower()}-{os.uname().machine}"


def release_names(project: str, version: str, platform: str) -> ReleaseNames:
    return ReleaseNames(
        source_tar=f'{project}-{version}-src.tar.gz',
        source_zip=f'{project}-{version}-src.zip',
        binary_tar=f'{project}-{version}-{platform}.tar.gz',
        binary_zip=f'{project}-{version}-{platform}.zip',
        source_sbom=f'{project}-{version}-source.cdx.json',
        binary_sbom=f'{project}-{version}-binary.cdx.json',
        manifest='manifest.json',
        sha256sums='SHA256SUMS',
        sha512sums='SHA512SUMS',
        provenance=f'{project}-{version}-provenance.intoto.jsonl',
        provenance_sig=f'{project}-{version}-provenance.intoto.jsonl.sig',
        manifest_sig='manifest.json.sig',
        public_key=f'{project}-{version}-release-public.pem',
        signing_meta=f'{project}-{version}-signing.json',
        policy='verification-policy.json',
    )


def should_exclude_source(rel: Path) -> bool:
    if any(part in EXCLUDED_DIR_NAMES for part in rel.parts):
        return True
    if rel.name in EXCLUDED_FILE_NAMES:
        return True
    if rel.suffix in EXCLUDED_SUFFIXES:
        return True
    rel_text = rel.as_posix()
    if rel.name == 'release-dev-private.pem':
        return True
    if rel_text.endswith('/build') and 'examples/cmake_consumer' in rel_text:
        return True
    if rel.name.startswith('lib') and '.so' in rel.name:
        return True
    if rel.name in {'libpcx.a', 'libbmp.a', 'pcx.pc', 'bmp.pc'}:
        return True
    if rel.parent.as_posix() in {'tools', 'tests'} and rel.suffix == '':
        if rel.name.startswith(('pcx_', 'bmp_', 'test_')):
            return True
    return False


def deterministic_mtime_tuple(epoch: int) -> Tuple[int, int, int, int, int, int]:
    dt = datetime.fromtimestamp(max(epoch, 315532800), tz=timezone.utc)
    return (dt.year, dt.month, dt.day, dt.hour, dt.minute, dt.second)


def iter_source_files(repo_root: Path) -> List[Path]:
    files: List[Path] = []
    for path in repo_root.rglob('*'):
        if should_exclude_source(path.relative_to(repo_root)):
            continue
        if path.is_file() or path.is_symlink():
            files.append(path.relative_to(repo_root))
    return sorted(files)


def iter_stage_files(stage_root: Path) -> List[Path]:
    files: List[Path] = []
    for path in stage_root.rglob('*'):
        rel = path.relative_to(stage_root)
        if any(part in EXCLUDED_DIR_NAMES for part in rel.parts):
            continue
        if path.is_file() or path.is_symlink():
            files.append(rel)
    return sorted(files)


def read_bytes(path: Path) -> bytes:
    if path.is_symlink():
        return os.readlink(path).encode('utf-8')
    return path.read_bytes()


def file_hashes(path: Path) -> Tuple[str, str]:
    data = read_bytes(path)
    return hashlib.sha256(data).hexdigest(), hashlib.sha512(data).hexdigest()


def add_directory_entries_tar(tf: tarfile.TarFile, topdir: str, rel_paths: Iterable[Path], epoch: int) -> None:
    seen = set()
    for rel in rel_paths:
        parents = [Path(topdir)]
        parents.extend(Path(topdir, *rel.parts[:i]) for i in range(1, len(rel.parts)))
        for parent in parents:
            parent_s = parent.as_posix().rstrip('/')
            if not parent_s or parent_s in seen:
                continue
            seen.add(parent_s)
            info = tarfile.TarInfo(parent_s + '/')
            info.type = tarfile.DIRTYPE
            info.mode = 0o755
            info.uid = 0
            info.gid = 0
            info.uname = 'root'
            info.gname = 'root'
            info.mtime = epoch
            tf.addfile(info)


def write_tar_gz(root: Path, rel_paths: List[Path], output_path: Path, topdir: str, epoch: int) -> None:
    output_path.parent.mkdir(parents=True, exist_ok=True)
    with output_path.open('wb') as raw:
        with gzip.GzipFile(filename='', mode='wb', fileobj=raw, mtime=epoch) as gz:
            with tarfile.open(fileobj=gz, mode='w') as tf:
                add_directory_entries_tar(tf, topdir, rel_paths, epoch)
                for rel in rel_paths:
                    src = root / rel
                    arcname = Path(topdir) / rel
                    info = tarfile.TarInfo(arcname.as_posix())
                    info.uid = 0
                    info.gid = 0
                    info.uname = 'root'
                    info.gname = 'root'
                    info.mtime = epoch
                    mode = src.lstat().st_mode
                    if src.is_symlink():
                        info.type = tarfile.SYMTYPE
                        info.mode = 0o777
                        info.linkname = os.readlink(src)
                        tf.addfile(info)
                    else:
                        info.mode = stat.S_IMODE(mode) or 0o644
                        data = src.read_bytes()
                        info.size = len(data)
                        import io
                        tf.addfile(info, io.BytesIO(data))


def write_zip(root: Path, rel_paths: List[Path], output_path: Path, topdir: str, epoch: int) -> None:
    output_path.parent.mkdir(parents=True, exist_ok=True)
    ts = deterministic_mtime_tuple(epoch)
    with zipfile.ZipFile(output_path, mode='w', compression=zipfile.ZIP_DEFLATED) as zf:
        seen_dirs = set()
        for rel in rel_paths:
            parts = [topdir]
            for part in rel.parts[:-1]:
                parts.append(part)
                dirname = '/'.join(parts) + '/'
                if dirname in seen_dirs:
                    continue
                seen_dirs.add(dirname)
                info = zipfile.ZipInfo(dirname, date_time=ts)
                info.create_system = 3
                info.external_attr = (0o40755 << 16) | 0x10
                zf.writestr(info, b'')
        for rel in rel_paths:
            src = root / rel
            arcname = f'{topdir}/{rel.as_posix()}'
            if src.is_symlink():
                info = zipfile.ZipInfo(arcname, date_time=ts)
                info.create_system = 3
                info.external_attr = 0o120777 << 16
                info.compress_type = zipfile.ZIP_STORED
                zf.writestr(info, os.readlink(src).encode('utf-8'))
            else:
                info = zipfile.ZipInfo(arcname, date_time=ts)
                info.create_system = 3
                info.external_attr = (src.stat().st_mode & 0xFFFF) << 16
                info.compress_type = zipfile.ZIP_DEFLATED
                zf.writestr(info, src.read_bytes())


def bom_serial(project: str, version: str, flavor: str) -> str:
    import uuid
    return f'urn:uuid:{uuid.uuid5(uuid.NAMESPACE_URL, f"{project}:{version}:{flavor}")}'


def build_component(path_root: Path, rel: Path, project: str, version: str) -> dict:
    full = path_root / rel
    sha256, sha512 = file_hashes(full)
    return {
        'type': 'file',
        'bom-ref': rel.as_posix(),
        'name': rel.name,
        'version': version,
        'hashes': [
            {'alg': 'SHA-256', 'content': sha256},
            {'alg': 'SHA-512', 'content': sha512},
        ],
        'properties': [
            {'name': 'codec:path', 'value': rel.as_posix()},
            {'name': 'codec:project', 'value': project},
        ],
    }


def generate_sbom(root: Path, rel_paths: List[Path], output_path: Path, project: str, version: str, flavor: str, epoch: int) -> None:
    root_component = {
        'type': 'library',
        'bom-ref': f'pkg:generic/{project}@{version}',
        'name': project,
        'version': version,
        'purl': f'pkg:generic/{project}@{version}',
    }
    document = {
        'bomFormat': 'CycloneDX',
        'specVersion': SBOM_SPEC_VERSION,
        'serialNumber': bom_serial(project, version, flavor),
        'version': 1,
        'metadata': {
            'timestamp': datetime.fromtimestamp(epoch, tz=timezone.utc).isoformat(),
            'component': root_component,
            'tools': {
                'components': [
                    {'type': 'application', 'name': 'codec-release-tooling', 'version': version}
                ]
            },
            'properties': [
                {'name': 'codec:flavor', 'value': flavor},
                {'name': 'codec:project', 'value': project},
            ],
        },
        'components': [build_component(root, rel, project, version) for rel in rel_paths],
    }
    output_path.write_text(json.dumps(document, indent=2, sort_keys=True) + '\n', encoding='utf-8')


def copy_docs(repo_root: Path, stage_root: Path, project: str, version: str) -> None:
    doc_root = stage_root / 'usr' / 'share' / 'doc' / project
    doc_root.mkdir(parents=True, exist_ok=True)
    for candidate in DOC_CANDIDATES:
        src = repo_root / candidate
        if src.exists():
            shutil.copy2(src, doc_root / candidate)
    version_file = repo_root / 'VERSION'
    if version_file.exists():
        shutil.copy2(version_file, doc_root / 'VERSION')
    release_notes = repo_root / f'RELEASE_{version.replace(".", "_")[:-2]}.md'
    if release_notes.exists():
        shutil.copy2(release_notes, doc_root / release_notes.name)


def write_checksum_file(paths: List[Path], output_path: Path, algorithm: str) -> None:
    lines = []
    for path in sorted(paths, key=lambda p: p.name):
        digest = getattr(hashlib, algorithm)(path.read_bytes()).hexdigest()
        lines.append(f'{digest}  {path.name}')
    output_path.write_text('\n'.join(lines) + '\n', encoding='utf-8')


def write_manifest(project: str, version: str, platform: str, epoch: int, files: List[Path], out_path: Path) -> None:
    artifacts = []
    for path in sorted(files, key=lambda p: p.name):
        sha256 = hashlib.sha256(path.read_bytes()).hexdigest()
        sha512 = hashlib.sha512(path.read_bytes()).hexdigest()
        artifacts.append({
            'name': path.name,
            'size': path.stat().st_size,
            'sha256': sha256,
            'sha512': sha512,
        })
    document = {
        'project': project,
        'version': version,
        'platform': platform,
        'source_date_epoch': epoch,
        'artifacts': artifacts,
    }
    out_path.write_text(json.dumps(document, indent=2, sort_keys=True) + '\n', encoding='utf-8')


def make_install(repo_root: Path, make_cmd: str, stage_root: Path, prefix: str) -> None:
    env = os.environ.copy()
    cmd = [make_cmd, 'install', f'DESTDIR={stage_root}', f'PREFIX={prefix}']
    subprocess.run(cmd, cwd=repo_root, check=True, env=env)


def ensure_public_key(openssl: str, private_key: Path, public_key: Path) -> None:
    if public_key.exists():
        return
    subprocess.run([openssl, 'pkey', '-in', str(private_key), '-pubout', '-out', str(public_key)], check=True)


def sign_file(openssl: str, private_key: Path, input_path: Path, output_path: Path) -> None:
    subprocess.run([
        openssl, 'dgst', '-sha256', '-sign', str(private_key), '-out', str(output_path), str(input_path)
    ], check=True)


def sha256_hex(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def build_provenance(project: str, version: str, platform: str, epoch: int, builder_id: str,
                     build_type: str, source_uri: str, invocation_id: str,
                     release_files: List[Path], manifest_path: Path, sha256_path: Path, sha512_path: Path,
                     prefix: str, make_cmd: str) -> dict:
    subject_targets = list(release_files) + [manifest_path, sha256_path, sha512_path]
    subject = [
        {'name': path.name, 'digest': {'sha256': sha256_hex(path)}}
        for path in sorted(subject_targets, key=lambda p: p.name)
    ]
    ts = datetime.fromtimestamp(epoch, tz=timezone.utc).isoformat()
    return {
        '_type': 'https://in-toto.io/Statement/v1',
        'subject': subject,
        'predicateType': 'https://slsa.dev/provenance/v1',
        'predicate': {
            'buildDefinition': {
                'buildType': build_type,
                'externalParameters': {
                    'project': project,
                    'version': version,
                    'platform': platform,
                    'prefix': prefix,
                    'make': make_cmd,
                },
                'internalParameters': {
                    'source_date_epoch': epoch,
                },
                'resolvedDependencies': [
                    {'uri': source_uri, 'digest': {'sha256': sha256_hex(manifest_path)}}
                ],
            },
            'runDetails': {
                'builder': {'id': builder_id},
                'metadata': {
                    'invocationId': invocation_id,
                    'startedOn': ts,
                    'finishedOn': ts,
                    'completeness': {
                        'parameters': True,
                        'environment': False,
                        'materials': True,
                    },
                    'reproducible': True,
                },
                'byproducts': [
                    {'name': manifest_path.name, 'digest': {'sha256': sha256_hex(manifest_path)}},
                    {'name': sha256_path.name, 'digest': {'sha256': sha256_hex(sha256_path)}},
                    {'name': sha512_path.name, 'digest': {'sha256': sha256_hex(sha512_path)}},
                ],
            },
        },
    }


def write_signing_metadata(out_path: Path, public_key_path: Path, mode: str, builder_id: str, build_type: str) -> None:
    meta = {
        'signatureAlgorithm': 'RSA-PKCS1v1.5-SHA256',
        'publicKeyFile': public_key_path.name,
        'publicKeySha256': sha256_hex(public_key_path),
        'mode': mode,
        'builderId': builder_id,
        'buildType': build_type,
    }
    out_path.write_text(json.dumps(meta, indent=2, sort_keys=True) + '\n', encoding='utf-8')


def write_policy(out_path: Path, project: str, version: str, platform: str, builder_id: str,
                 build_type: str, names: ReleaseNames, release_files: List[Path]) -> None:
    doc = {
        'version': 1,
        'project': project,
        'trusted_builder_ids': [builder_id],
        'expected_build_type': build_type,
        'expected_external_parameters': {
            'project': project,
            'version': version,
            'platform': platform,
        },
        'required_subjects': sorted([p.name for p in release_files] + [names.manifest, names.sha256sums, names.sha512sums]),
        'public_key': names.public_key,
        'provenance': names.provenance,
        'provenance_signature': names.provenance_sig,
        'manifest_signature': names.manifest_sig,
    }
    out_path.write_text(json.dumps(doc, indent=2, sort_keys=True) + '\n', encoding='utf-8')


def main() -> int:
    parser = argparse.ArgumentParser(description='Build deterministic release artifacts, SBOMs, checksums, provenance, and detached signatures.')
    parser.add_argument('--project', required=True)
    parser.add_argument('--version', required=True)
    parser.add_argument('--repo-root', required=True)
    parser.add_argument('--dist-dir', default='dist')
    parser.add_argument('--platform-tag', default=None)
    parser.add_argument('--make', default='make')
    parser.add_argument('--prefix', default='/usr')
    parser.add_argument('--only', choices=['all', 'sbom'], default='all')
    parser.add_argument('--openssl', default='openssl')
    parser.add_argument('--signing-key', default=None)
    parser.add_argument('--signing-public-key', default=None)
    parser.add_argument('--builder-id', default=DEFAULT_BUILDER_ID)
    parser.add_argument('--build-type', default=DEFAULT_BUILD_TYPE)
    parser.add_argument('--source-uri', default=None)
    parser.add_argument('--invocation-id', default=None)
    args = parser.parse_args()

    repo_root = Path(args.repo_root).resolve()
    dist_dir = (repo_root / args.dist_dir).resolve()
    release_root = (repo_root / '.release').resolve()
    stage_root = release_root / 'stage'
    release_root.mkdir(parents=True, exist_ok=True)
    if dist_dir.exists():
        shutil.rmtree(dist_dir)
    dist_dir.mkdir(parents=True, exist_ok=True)
    if stage_root.exists():
        shutil.rmtree(stage_root)
    stage_root.mkdir(parents=True, exist_ok=True)

    epoch = stable_epoch(repo_root)
    platform = args.platform_tag or platform_tag()
    names = release_names(args.project, args.version, platform)

    source_files = iter_source_files(repo_root)
    source_topdir = f'{args.project}-{args.version}'
    source_tar = dist_dir / names.source_tar
    source_zip = dist_dir / names.source_zip
    write_tar_gz(repo_root, source_files, source_tar, source_topdir, epoch)
    write_zip(repo_root, source_files, source_zip, source_topdir, epoch)
    generate_sbom(repo_root, source_files, dist_dir / names.source_sbom, args.project, args.version, 'source', epoch)

    if args.only == 'all':
        make_install(repo_root, args.make, stage_root, args.prefix)
        copy_docs(repo_root, stage_root, args.project, args.version)
        stage_files = iter_stage_files(stage_root)
        binary_topdir = f'{args.project}-{args.version}-{platform}'
        binary_tar = dist_dir / names.binary_tar
        binary_zip = dist_dir / names.binary_zip
        write_tar_gz(stage_root, stage_files, binary_tar, binary_topdir, epoch)
        write_zip(stage_root, stage_files, binary_zip, binary_topdir, epoch)
        generate_sbom(stage_root, stage_files, dist_dir / names.binary_sbom, args.project, args.version, 'binary', epoch)
        release_files = [
            source_tar, source_zip, binary_tar, binary_zip,
            dist_dir / names.source_sbom, dist_dir / names.binary_sbom,
        ]
    else:
        release_files = [dist_dir / names.source_sbom]

    manifest_path = dist_dir / names.manifest
    sha256_path = dist_dir / names.sha256sums
    sha512_path = dist_dir / names.sha512sums
    write_manifest(args.project, args.version, platform, epoch, release_files, manifest_path)
    checksum_targets = release_files + [manifest_path]
    write_checksum_file(checksum_targets, sha256_path, 'sha256')
    write_checksum_file(checksum_targets, sha512_path, 'sha512')

    fixture_priv = repo_root / 'tests' / 'fixtures' / 'release-dev-private.pem'
    fixture_pub = repo_root / 'tests' / 'fixtures' / 'release-dev-public.pem'
    signing_key = Path(args.signing_key).resolve() if args.signing_key else fixture_priv
    signing_public = Path(args.signing_public_key).resolve() if args.signing_public_key else fixture_pub
    if not signing_key.exists():
        raise SystemExit(f'missing signing key: {signing_key}')
    ensure_public_key(args.openssl, signing_key, signing_public)
    mode = 'external-key' if args.signing_key else 'development-fixture'

    public_key_out = dist_dir / names.public_key
    shutil.copy2(signing_public, public_key_out)

    provenance_path = dist_dir / names.provenance
    provenance = build_provenance(
        args.project,
        args.version,
        platform,
        epoch,
        args.builder_id,
        args.build_type,
        args.source_uri or f'{DEFAULT_SOURCE_URI_SCHEME}{repo_root.name}',
        args.invocation_id or f'{args.project}-{args.version}-{platform}-release',
        release_files,
        manifest_path,
        sha256_path,
        sha512_path,
        args.prefix,
        args.make,
    )
    provenance_path.write_text(json.dumps(provenance, indent=2, sort_keys=True) + '\n', encoding='utf-8')

    sign_file(args.openssl, signing_key, manifest_path, dist_dir / names.manifest_sig)
    sign_file(args.openssl, signing_key, provenance_path, dist_dir / names.provenance_sig)
    write_signing_metadata(dist_dir / names.signing_meta, public_key_out, mode, args.builder_id, args.build_type)
    write_policy(dist_dir / names.policy, args.project, args.version, platform, args.builder_id, args.build_type, names, release_files)
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
