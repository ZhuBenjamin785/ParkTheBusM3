"""Run from anywhere with Python 3 and a C++17 compiler on PATH."""
from pathlib import Path
from tempfile import TemporaryDirectory
import subprocess
import os

root = Path(__file__).resolve().parent.parent
mocks = root / "checks" / "simulation"
with TemporaryDirectory() as temp:
    for name, sketch in (("ring", "LineNode"), ("main", "MainControl")):
        binary = Path(temp) / name
        subprocess.run([os.environ.get("CXX", "c++"), "-std=c++17", "-I", str(mocks),
                        "-I", str(root / "firmware" / sketch), str(mocks / (name + "-test.cpp")),
                        "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True)
        print(sketch, "passed")
