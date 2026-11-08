"""Build and execute the isolated TVS_ALIGN plan geometry test; no AutoCAD SDK required."""
import pathlib
import subprocess
import tempfile
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]

class AlignGeometryTests(unittest.TestCase):
    def test_cpp_geometry(self):
        with tempfile.TemporaryDirectory() as folder:
            binary = pathlib.Path(folder) / 'align_geometry'
            subprocess.run(['g++', '-std=c++14', '-Wall', '-Wextra', '-Werror', '-pedantic',
                            str(ROOT / 'tests/test_align_geometry.cpp'), '-o', str(binary)], check=True)
            subprocess.run([str(binary)], check=True)

if __name__ == '__main__':
    unittest.main(verbosity=2)
