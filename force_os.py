from typing import Any, TYPE_CHECKING

if TYPE_CHECKING:
    Import: Any = None
    env: Any = {}

Import("env")

# Substituir todas as flags de otimização existentes por -Os
for scope in ("CCFLAGS", "CXXFLAGS"):
    flags = env[scope]
    for i, flag in enumerate(flags):
        if flag in ("-O0", "-O1", "-O2", "-O3", "-Og", "-Os"):
            flags[i] = "-Os"