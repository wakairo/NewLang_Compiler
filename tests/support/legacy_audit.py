"""Opt-in switch only for identified legacy hash/history substeps (C293)."""
import os


def enabled():
    value = os.environ.get('NEWLANG_LEGACY_AUDIT', 'OFF')
    if value not in ('OFF', 'ON', '0', '1'):
        raise ValueError('NEWLANG_LEGACY_AUDIT must be OFF/ON/0/1')
    return value in ('ON', '1')
