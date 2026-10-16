# R524 frozen passive GDB observer. Reads, never changes target guest state.
import gdb
import os
from pathlib import Path

source = Path(os.environ['R524_CFILE']).resolve()
lines = source.read_text(encoding='utf-8').splitlines()
count = [0]

class PassiveSnapshot(gdb.Breakpoint):
    def __init__(self, line):
        super().__init__(f'{source}:{line}', internal=False)
        self.line = line

    def stop(self):
        count[0] += 1
        if count[0] > 250:
            print('R524_PASSIVE_SNAPSHOT_CAP_REACHED')
            return False
        frame = gdb.selected_frame()
        sal = frame.find_sal()
        print(f'R524_SNAPSHOT step={count[0]} source_line={self.line} frame={frame.name()} actual_line={sal.line}')
        try:
            block = frame.block()
            seen = set()
            while block is not None and not block.is_global and not block.is_static:
                for sym in block:
                    if not (sym.is_variable or sym.is_argument) or sym.name in seen:
                        continue
                    seen.add(sym.name)
                    try:
                        value = sym.value(frame)
                        addr = value.address
                        print(f'  {sym.name}: type={value.type}, value={value}, address={addr}')
                        typ = value.type.strip_typedefs()
                        if typ.code == gdb.TYPE_CODE_STRUCT:
                            for field in typ.fields():
                                if field.name:
                                    child = value[field.name]
                                    print(f'    .{field.name}: value={child}, address={child.address}')
                    except Exception as exc:
                        print(f'  {sym.name}: UNAVAILABLE ({type(exc).__name__}: {exc})')
                block = block.superblock
        except Exception as exc:
            print(f'R524_SNAPSHOT_FAILURE: {type(exc).__name__}: {exc}')
        return False

gdb.execute('set pagination off')
gdb.execute('set confirm off')
gdb.execute('set print static off')
for i, line in enumerate(lines, 1):
    if any(c in line for c in (';', '(', '}', '{')) and not line.lstrip().startswith('#'):
        try:
            PassiveSnapshot(i)
        except gdb.error:
            pass
print(f'R524_PASSIVE_BREAKPOINTS_INSTALLED source={source}')
gdb.execute('run')
print(f'R524_PASSIVE_EXEC_COMPLETE snapshots={count[0]}')
