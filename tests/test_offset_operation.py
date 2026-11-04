"""Shared actual planning code; not an ObjectARX transaction/UCS runtime test."""
import pathlib
import subprocess
import tempfile
import unittest
from bypass_compiler import compile_cpp, executable

ROOT = pathlib.Path(__file__).resolve().parents[1]

class OffsetOperationTests(unittest.TestCase):
    def test_shared_physical_plan(self):
        with tempfile.TemporaryDirectory() as folder:
            target = pathlib.Path(folder) / 'offset-operation'
            compile_cpp(ROOT / 'tests/test_offset_operation.cpp', target)
            subprocess.run([str(executable(target))], check=True)

if __name__ == '__main__':
    unittest.main(verbosity=2)
