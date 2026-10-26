set pagination off
set confirm off
set auto-load off
set debuginfod enabled off
set disable-randomization off
source /tmp/p932-gate-a/ast-observe.py
run
python
if errors: raise gdb.GdbError(repr(errors))
end
