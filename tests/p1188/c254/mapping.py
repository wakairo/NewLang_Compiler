"""World-qualified source roles, then C initialization/use order before native."""
from collections import Counter
import re


def roles(text):
    m = re.search(r'C254 MAP loop=(\d+) phase=(\d+) result=(\d+) base=(\d+) index=(\d+) helper=(\d+)', text)
    assert m
    d = dict(zip(['loop', 'phase', 'result', 'base', 'index', 'helper'], map(int, m.groups())))
    d['prefix'] = {int(s): dict(name=n, kind=k, source=int(a), index=int(i)) for s, n, k, a, i in re.findall(
        r'C254 PREFIX symbol=(\d+) name=(\w+) kind=(\w+) from=(\d+) index=(\d+)', text)}
    d['locals'] = {int(s): dict(name=n, quantity=int(q)) for s, n, q in re.findall(
        r'C254 LOCAL symbol=(\d+) name=(\w+) quantity=(\d+)', text)}
    d['uses'] = [dict(node=int(n), symbol=int(s), statement=int(t)) for n, s, t in re.findall(
        r'C254 CAPTURE node=(\d+) symbol=(\d+) statement=(\d+)', text)]
    assert 'C254 OWNED public-body=YES private-current=YES canonical-root=YES' in text
    return d


def inspect(text, view, meta):
    assert view['phase'] == view['result']
    declaration = f"size_t nl_local_{view['result']};"
    start = text.index('for (;;) {', text.index(declaration))
    stop = text.index(f"nl_local_{view['result']} = nl_loop_phase_{view['loop']};", start)
    body = text[start:stop]
    assert not re.search(r'\bnl_local_' + str(view['result']) + r'\b', body), 'uninitialized caller result read'
    counts = Counter(u['symbol'] for u in view['uses'])
    values = {}
    last = 0
    for symbol, p in view['prefix'].items():
        assert symbol < view['phase']
        name = f'nl_local_{symbol}'
        pattern = r'const uint8_t ' + name + r' = ([^;\n]+);'
        match = re.search(pattern, text[:start])
        assert match and match.start() > last, ('outer initialization order', p['name'])
        last = match.start()
        if p['kind'] == 'copy':
            expected = f"nl_local_{p['source']}"
            assert p['source'] in values
            values[symbol] = values[p['source']]
        elif p['kind'] == 'field':
            expected = f"nl_local_{view['base']}.f{p['index']}"
            values[symbol] = meta['initial'] if p['index'] == meta['index'] else meta['peer']
        else:
            assert p['kind'] == 'literal'
            expected = str(meta['captured'])
            values[symbol] = meta['captured']
        assert match[1].strip() == expected, ('source initializer replaced', p, match[1], expected)
        assert not re.search(pattern, body), ('outer value reinitialized inside iteration', p)
        actual = len(re.findall(r'\b' + name + r'\b', body))
        assert actual == counts[symbol], ('outer carrier use', p, actual, counts[symbol])
    assert {p['name'] for p in view['locals'].values()} == set(meta['locals'])
    assert bool(view['uses']) == meta['new_capture_use']
    return dict(start=start, stop=stop, body=body, prefix_values=values,
                direct_discards=[u for u in view['uses'] if u['statement']])
