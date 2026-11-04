"""Build and execute the isolated C++98 geometry test; no AutoCAD SDK required."""
import pathlib
import subprocess
import tempfile
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]

class OffsetGeometryTests(unittest.TestCase):
    def test_cpp98_geometry(self):
        with tempfile.TemporaryDirectory() as folder:
            binary = pathlib.Path(folder) / 'offset_geometry'
            subprocess.run(['g++', '-std=c++98', '-Wall', '-Wextra', '-Werror', '-pedantic',
                            str(ROOT / 'tests/test_offset_geometry.cpp'), '-o', str(binary)], check=True)
            subprocess.run([str(binary)], check=True)

if __name__ == '__main__':
    unittest.main(verbosity=2)
