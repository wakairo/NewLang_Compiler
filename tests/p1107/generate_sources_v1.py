"""P-owned canonical examples; no R source, oracle, runner or raw inputs."""
from pathlib import Path
import json

root = Path(__file__).parent
(root / 'sources').mkdir(exist_ok=True)
helper = '''fn exchange_sample(destination: ref<write, u8>, incoming: u8) -> u8 {
    let displaced = replace(destination, incoming);
    let forwarded = displaced;
    return forwarded;
}
'''
start = 'struct PulsePair { spare: u8, active: u8 }\n'
primary = start + helper + '''fn main() -> unit {
    let pair = PulsePair{spare: u8(64), active: u8(185)};
    let recovered = loop () {
        let previous = loan_write(pair@active) { |channel| exchange_sample(channel, u8(226)) };
        break previous;
    };
    let spare_old = loan_write(pair@spare) { |channel| replace(channel, recovered) };
    let active_now = pair@active;
    let spare_now = pair@spare;
    return unit;
}
'''
inline = start + helper.replace('    let displaced = replace(destination, incoming);\n    let forwarded = displaced;\n    return forwarded;', '    return replace(destination, incoming);') + '''fn main() -> unit {
    let pair = PulsePair{spare: u8(153), active: u8(78)};
    let recovered = loop () {
        break loan_write(pair@spare) { |channel| exchange_sample(channel, u8(203)) };
    };
    let active_old = loan_write(pair@active) { |channel| replace(channel, recovered) };
    let active_now = pair@active;
    let spare_now = pair@spare;
    return unit;
}
'''
no_write = primary.replace('u8(64)', 'u8(67)').replace('u8(185)', 'u8(214)').replace('u8(226)', 'u8(91)').replace('    let spare_old = loan_write(pair@spare) { |channel| replace(channel, recovered) };\n', '').replace('    return forwarded;', '    forwarded')
two_roots = start + '''fn main() -> unit {
    let first = PulsePair{active: u8(111), spare: u8(32)};
    let second = PulsePair{active: u8(84), spare: u8(205)};
    let recovered = loop () {
        let previous = loan_write(second@spare) { |channel| exchange_sample(channel, u8(139)) };
        break previous;
    };
    let active_old = loan_write(second@active) { |channel| replace(channel, recovered) };
    let first_spare = first@spare;
    let first_active = first@active;
    let second_spare = second@spare;
    let second_active = second@active;
    return unit;
}
''' + helper
outside = primary.replace('    let recovered = loop () {\n        let previous = loan_write(pair@active) { |channel| exchange_sample(channel, u8(226)) };\n        break previous;\n    };', '    let recovered = loan_write(pair@active) { |channel| exchange_sample(channel, u8(226)) };')
primitive = primary.replace('exchange_sample(channel, u8(226))', 'replace(channel, u8(226))')
cases = {
    '01-bound': (primary, 3, 0, {'root':1,'index':1,'old':185,'new':226,'sibling':64,'after':[185,226]}),
    '02-inline': (inline, 3, 0, {'root':1,'index':0,'old':153,'new':203,'sibling':78,'after':[203,153]}),
    '03-no-caller-write': (no_write, 3, 0, {'root':1,'index':1,'old':214,'new':91,'sibling':67,'after':[67,91]}),
    '04-two-originals': (two_roots, 3, 0, {'root':2,'index':0,'old':205,'new':139,'sibling':84,'after':[139,205],'other':[32,111]}),
    '05-equal-bits-old': (primary.replace('u8(226)', 'u8(185)'), 3, 0, {'root':1,'index':1,'old':185,'new':185,'sibling':64,'after':[185,185]}),
    '06-outside-control': (outside, 0, 0, None),
    '07-primitive-control': (primitive, 0, 0, {'root':1,'index':1,'old':185,'new':226,'sibling':64,'after':[185,226]}),
    '08-equal-bits-wrong-return': (primary.replace('    return forwarded;', '    return incoming;').replace('u8(226)', 'u8(185)'), 3, 3, None),
    '09-read-loan-write': (primary.replace('loan_write(pair@active)', 'loan_read(pair@active)'), 3, 3, None),
    '10-multiple-changes': (primary.replace('    let forwarded = displaced;', '    let additional = replace(destination, u8(97));\n    let forwarded = displaced;'), 3, 3, None),
}
oracle = {'authority':'c6fe0b3493fb4012b07bfd8c2c17f79566b67ad4','canonical':'Draft17.34','cases':{}}
for name,(source,before,after,expected) in cases.items():
    (root / 'sources' / (name+'.nl')).write_text(source)
    oracle['cases'][name] = {'baseline':before,'candidate':after,'expected':expected,
                            'helper':name.startswith(('01','02','03','04','05'))}
(root / 'oracle-v1.json').write_text(json.dumps(oracle,indent=2)+'\n')
