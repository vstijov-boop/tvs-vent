"""Section combinations (rect/round route and obstacle, oblique crossing, envelope); SDK-free."""
import pathlib, subprocess, tempfile, unittest
from bypass_compiler import compile_cpp, executable
class BypassSections(unittest.TestCase):
    def test_section_combinations(self):
        root=pathlib.Path(__file__).resolve().parents[1]
        with tempfile.TemporaryDirectory() as folder:
            target=executable(pathlib.Path(folder)/"sections")
            compile_cpp(root/"tests/test_bypass_sections.cpp",target)
            subprocess.run([str(target)],check=True,timeout=600)
if __name__=="__main__":unittest.main(verbosity=2)
