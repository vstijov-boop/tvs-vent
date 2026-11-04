"""Pure storage scalar/quantity and projection tests; NOT XRecord/COPY runtime tests."""
import pathlib
import subprocess
import tempfile
import unittest
ROOT = pathlib.Path(__file__).resolve().parents[1]
class OffsetIntegrationTests(unittest.TestCase):
    def test_cpp98_record_quantity_projection(self):
        with tempfile.TemporaryDirectory() as folder:
            binary = pathlib.Path(folder) / 'integration'
            subprocess.run(['g++','-std=c++98','-Wall','-Wextra','-Werror','-pedantic',
                            str(ROOT/'tests/test_offset_integration.cpp'),'-o',str(binary)],check=True)
            subprocess.run([str(binary)],check=True)
if __name__ == '__main__':
    unittest.main(verbosity=2)
