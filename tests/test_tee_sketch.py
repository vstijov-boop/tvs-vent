"""TVS_TK_TIP sketch: every case builds and writes an SVG (tests/tee_sketch_svg.cpp); no AutoCAD SDK."""
import pathlib, subprocess, tempfile, unittest
ROOT = pathlib.Path(__file__).resolve().parents[1]
class TeeSketch(unittest.TestCase):
    def test_all_cases_render(self):
        with tempfile.TemporaryDirectory() as d:
            exe = pathlib.Path(d) / 'sketch'
            subprocess.run(['g++', '-std=c++14', '-Wall', str(ROOT / 'tests/tee_sketch_svg.cpp'), '-o', str(exe)], check=True)
            subprocess.run([str(exe), d], check=True)
            files = sorted(pathlib.Path(d).glob('*.svg'))
            self.assertEqual(len(files), 42)
            for f in files:
                self.assertIn('<polygon', f.read_text())
if __name__ == '__main__':
    unittest.main(verbosity=2)
