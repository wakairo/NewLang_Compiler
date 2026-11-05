"""Authenticate roles before any native compilation or execution."""
from collections import Counter
import re


def roles(trace):
    match = re.search(r'C250a MAP loop=(\d+) phase=(\d+) result=(\d+) index=(\d+) helper=(\d+)', trace)
    assert match, trace
    view = dict(zip(['loop', 'phase', 'result', 'index', 'helper'], map(int, match.groups())))
    view['locals'] = {name: dict(symbol=int(symbol), quantity=int(quantity)) for symbol, name, quantity in re.findall(
        r'C250a LOCAL symbol=(\d+) name=(\w+) quantity=(\d+)', trace)}
    view['discards'] = [dict(node=int(node), symbol=int(symbol), phase=int(phase)) for node, symbol, phase in re.findall(
        r'C250a DISCARD node=(\d+) symbol=(\d+) phase=(\d+)', trace)]
    assert 'C250a OWNED current=YES' in trace
    return view


def inspect(c_text, view, expected_statements, safe):
    """Result and phase have distinct world roles even when IDs coincide."""
    loop, result = view['loop'], view['result']
    assert view['phase'] == result
    declaration = f'size_t nl_local_{result};'
    start = c_text.index('for (;;) {', c_text.index(declaration))
    assignment = f'nl_local_{result} = nl_loop_phase_{loop};'
    stop = c_text.index(assignment, start)
    body = c_text[start:stop]
    bad = len(re.findall(r'\bnl_local_' + str(result) + r'\b', body))
    assert len(view['discards']) == expected_statements
    source_counts = Counter(
        f'nl_loop_phase_{loop}' if item['phase'] else f"nl_local_{item['symbol']}"
        for item in view['discards'])
    binding_counts = Counter(f"nl_local_{item['symbol']}" for item in view['locals'].values() if item['quantity'])
    counts = source_counts + binding_counts
    if safe:
        assert bad == 0, ('STOP result local read before Break assignment', result)
        for carrier, count in counts.items():
            assert body.count('(void)' + carrier + ';') == count, (carrier, count, body)
    return dict(bad_phase_reads=bad, body=body, carriers=dict(counts), start=start, stop=stop)
