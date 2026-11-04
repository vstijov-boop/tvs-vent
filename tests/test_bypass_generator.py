"""Execute the SDK-free generator; persist reference solutions when requested."""
import json,os,pathlib,subprocess,tempfile,unittest
from bypass_compiler import compile_cpp,executable
class BypassGenerator(unittest.TestCase):
    def test_verified_candidate_generation(self):
        root=pathlib.Path(__file__).resolve().parents[1]
        with tempfile.TemporaryDirectory() as folder:
            target=executable(pathlib.Path(folder)/"generator")
            compile_cpp(root/"tests/test_bypass_generator.cpp",target)
            result=subprocess.run([str(target)],capture_output=True,text=True,timeout=300)
            print(result.stdout,flush=True)
            if result.stderr:print(result.stderr,flush=True)
            output=os.environ.get("TVS_BYPASS_REFERENCE_OUT")
            if output:
                path=pathlib.Path(output);path.parent.mkdir(parents=True,exist_ok=True)
                cases=[json.loads(s[len("REFERENCE "):]) for s in result.stdout.splitlines() if s.startswith("REFERENCE ")]
                path.write_text(json.dumps(cases,indent=2)+"\n",encoding="utf-8")
            result.check_returncode()
if __name__=="__main__":unittest.main(verbosity=2)
