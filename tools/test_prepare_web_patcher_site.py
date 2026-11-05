"""Regression checks for release scope, asset paths and immutable payloads."""
import hashlib
import json
import os
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

import prepare_web_patcher_site as prepare


def manifest():
    data = b'verified payload'
    descriptor = dict(size=len(data), decoded_size=len(data), sha256=hashlib.sha256(data).hexdigest())
    return dict(schema=10, version='v0.1.1', distribution_scope='public-patch-data',
                parts={name: dict(descriptor) for name in prepare.PARTS})


class PreparationTests(unittest.TestCase):
    def test_private_repository_is_rejected_before_download(self):
        repository = dict(id=1424368097, private=True)
        with patch.object(prepare, 'command', return_value=json.dumps(repository)) as request:
            with self.assertRaisesRegex(ValueError, 'repository identity'):
                prepare.download_release('v0.1.1', Path('unused'))
            self.assertEqual(request.call_count, 1)

    def test_draft_or_wrong_author_is_rejected_before_download(self):
        repository = json.dumps(dict(id=1424368097, private=False))
        for override in (dict(draft=True), dict(author=dict(login='another-owner')), dict(published_at=None)):
            release = dict(draft=False, author=dict(login='NiceLikeTryce'),
                           tag_name='v0.1.1', published_at='2026-11-05T00:00:00Z')
            release.update(override)
            with self.subTest(override=override), patch.object(
                    prepare, 'command', side_effect=[repository, json.dumps(release)]) as request:
                with self.assertRaisesRegex(ValueError, 'published owner release'):
                    prepare.download_release('v0.1.1', Path('unused'))
                self.assertEqual(request.call_count, 2)

    def test_local_scope_requires_explicit_local_mode(self):
        record = manifest()
        record.update(version='beta-rc4', distribution_scope='owner-local')
        with self.assertRaises(ValueError):
            prepare.validate_manifest(record)
        self.assertIs(prepare.validate_manifest(record, allow_local=True), record)

    def test_extra_asset_cannot_escape_output_directory(self):
        record = manifest()
        record['parts']['../private.gba'] = dict(next(iter(record['parts'].values())))
        with self.assertRaisesRegex(ValueError, 'asset names'):
            prepare.validate_manifest(record)

    def test_boolean_size_is_rejected(self):
        record = manifest()
        next(iter(record['parts'].values()))['size'] = True
        with self.assertRaisesRegex(ValueError, 'asset size'):
            prepare.validate_manifest(record)

    def test_same_size_corruption_is_rejected(self):
        with tempfile.TemporaryDirectory(dir=os.environ.get('FRE_TEST_TMP')) as directory:
            root = Path(directory)
            source = root / 'asset.gz'
            source.write_bytes(b'corrupt! payload')
            descriptor = next(iter(manifest()['parts'].values()))
            self.assertEqual(source.stat().st_size, descriptor['size'])
            with self.assertRaisesRegex(ValueError, 'hash differs'):
                prepare.copy_asset(source, root / 'copied.gz', descriptor)
            self.assertEqual(source.read_bytes(), b'corrupt! payload')


if __name__ == '__main__':
    unittest.main()
