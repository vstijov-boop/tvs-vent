"""SDK-free candidate checker and existing regression suite; no AutoCAD runtime."""
import pathlib, subprocess, tempfile, unittest
from bypass_compiler import compile_cpp, executable
class BypassCandidate(unittest.TestCase):
    def test_candidate_and_solid_distance(self):
        root=pathlib.Path(__file__).resolve().parents[1]
        with tempfile.TemporaryDirectory() as folder:
            target=executable(pathlib.Path(folder)/"bypass")
            compile_cpp(root/"tests/test_bypass_candidate.cpp",target)
            subprocess.run([str(target)],check=True,timeout=180)
if __name__=="__main__":unittest.main(verbosity=2)
