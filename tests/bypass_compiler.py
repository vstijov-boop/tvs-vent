"""Compiler selection for isolated bypass tests only; no project flags are modified."""
import os
import pathlib
import subprocess

def executable(path):
    path=pathlib.Path(path)
    return path.with_suffix(".exe") if os.name=="nt" else path

def msvc():
    return os.environ.get("TVS_BYPASS_CXX","").lower()=="msvc"

def compile_cpp(source,target,fast=False,syntax_only=False):
    source,target=pathlib.Path(source),executable(target)
    if msvc():
        command=["cl","/nologo","/EHsc","/std:c++14","/O2",
                 "/fp:fast" if fast else "/fp:strict","/W4","/WX"]
        command += ["/Zs"] if syntax_only else ["/Fe"+str(target),"/Fo"+str(target.with_suffix(".obj"))]
        command += [str(source)]
    else:
        command=["g++","-std=c++11","-O2","-ffast-math" if fast else "-fno-fast-math",
                 "-ffp-contract=off","-Wall","-Wextra","-Werror","-pedantic",str(source)]
        command += ["-fsyntax-only"] if syntax_only else ["-o",str(target)]
    print("COMPILE:",subprocess.list2cmdline(command),flush=True)
    result=subprocess.run(command,capture_output=True,text=True)
    if result.stdout:print(result.stdout,flush=True)
    if result.stderr:print(result.stderr,flush=True)
    if not fast:result.check_returncode()
    return result
