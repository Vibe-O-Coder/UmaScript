"""Temporary test workspaces with inherited ACLs on Windows."""
from contextlib import contextmanager
from pathlib import Path
import shutil
import uuid

@contextmanager
def temporary_directory(prefix='uma-test-'):
    root = (Path(__file__).resolve().parent.parent / '.test-tmp').resolve()
    root.mkdir(exist_ok=True)
    path = root / (prefix + uuid.uuid4().hex)
    path.mkdir()
    try:
        yield str(path)
    finally:
        resolved = path.resolve()
        if resolved.parent != root:
            raise RuntimeError('Test cleanup escaped its workspace')
        shutil.rmtree(resolved)
