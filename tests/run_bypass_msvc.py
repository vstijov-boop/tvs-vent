"""Configure the existing development toolset in this subprocess, then RUN tests."""
import os,pathlib,subprocess,sys
toolset=os.environ["TVS_BYPASS_TOOLSET"]
if toolset=="v141":
    root=pathlib.Path(os.environ["TVS_VS2017"])
    args="10.0.17763.0 -vcvars_ver=14.16"
else:
    root=pathlib.Path(r"C:\Program Files\Microsoft Visual Studio\2022\Enterprise")
    args="-vcvars_ver=14.29"
batch=root/"VC/Auxiliary/Build/vcvars64.bat"
command='call "'+str(batch)+'" '+args+' >nul && set'
configured=subprocess.run(command,shell=True,capture_output=True,text=True,check=True)
env=os.environ.copy()
for line in configured.stdout.splitlines():
    if "=" in line and not line.startswith("="):
        key,value=line.split("=",1);env[key]=value
env["TVS_BYPASS_CXX"]="msvc"
print("TOOLSET",toolset,"vcvars",batch,args,flush=True)
for name in ["VCToolsInstallDir","VCToolsVersion","WindowsSDKVersion","WindowsSdkDir"]:
    print(name,env.get(name),flush=True)
version=subprocess.run(["cl"],env=env,capture_output=True,text=True)
print(version.stdout+version.stderr,flush=True)
result=subprocess.run([sys.executable,"-m","unittest","discover","-s","tests","-p","test_bypass*.py","-v"],env=env)
sys.exit(result.returncode)
