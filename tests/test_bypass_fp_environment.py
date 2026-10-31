import pathlib,subprocess,tempfile,unittest
from bypass_compiler import compile_cpp,executable
class BypassEnvironment(unittest.TestCase):
    def test_floating_environment(self):
        root=pathlib.Path(__file__).resolve().parents[1]
        with tempfile.TemporaryDirectory() as directory:
            target=executable(pathlib.Path(directory)/"fp")
            compile_cpp(root/"tests/test_bypass_fp_environment.cpp",target)
            subprocess.run([str(target)],check=True,timeout=180)
if __name__=="__main__":unittest.main(verbosity=2)
