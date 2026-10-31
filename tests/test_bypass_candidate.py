"""SDK-free candidate checker and existing regression suite; no AutoCAD runtime."""
import pathlib, subprocess, tempfile, unittest
class BypassCandidate(unittest.TestCase):
    def test_candidate_and_solid_distance(self):
        root=pathlib.Path(__file__).resolve().parents[1]
        with tempfile.TemporaryDirectory() as folder:
            target=pathlib.Path(folder)/"bypass"
            subprocess.run(["g++","-std=c++11","-O2","-fno-fast-math","-ffp-contract=off",
                "-Wall","-Wextra","-Werror","-pedantic",str(root/"tests/test_bypass_candidate.cpp"),"-o",str(target)],check=True)
            subprocess.run([str(target)],check=True,timeout=180)
if __name__=="__main__":unittest.main(verbosity=2)
