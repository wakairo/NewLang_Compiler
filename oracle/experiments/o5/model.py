"""Synthetic, non-authoritative O5 vocabulary.  NOT a NewLang parser or C issuer.

Identities are testcase hypotheses, never grants of source Allocation/Domain rights.
Instances of this module are the only shared objects between independent validators.
No transition functions and no acceptance logic are defined here.
"""
from __future__ import annotations
from dataclasses import dataclass
import hashlib
import json
from dataclasses import asdict


@dataclass(frozen=True)
class Original:
    name: str
    backing: str
    incarnation: str
    allocation: str
    domain: str
    owner_binding: str
    allocation_live: bool = True
    domain_live: bool = True


@dataclass(frozen=True)
class Value:
    kind: str             # Option | u8
    pointer_root: str | None = None
    scalar: int | None = None
    pointer_backing: str | None = None
    pointer_incarnation: str | None = None
    carries_allocation: bool = False
    carries_domain: bool = False


@dataclass(frozen=True)
class Cell:
    place: str            # fixed-subobject identity
    incarnation: str      # survives whole-field Change/Reset
    value: Value
    fact: str             # changes even if same numerical scalar value
    occurrence: str | None = None


@dataclass(frozen=True)
class Step:
    sid: str              # synthetic source operation, unique within one program
    kind: str             # change | copy | unknown | owner_move
    root: str = ""
    field: int = -1       # indices 0,1,2 = Option; 3 = u8
    value: Value | None = None
    read_domain: str = ""  # READ stability loan is domain-scoped; NOT a write loan
    read_access: bool = True
    write_ref: bool = True
    ref_root: str = ""
    ref_incarnation: str = ""
    projection: int = -1
    scope: str = ""
    derived_scope: str = ""
    scope_closed: bool = True
    effect: str = "known"


@dataclass(frozen=True)
class Witness:
    """Claimed checked event; separate from the *source* step tape.

    before/after are full snapshots of synthetic field currentness (root and cells).
    They do not authenticate their producer and are not original A/D authority.
    """
    ordinal: int
    sid: str
    source_index: int
    before: tuple[tuple[str, object], ...]
    after: tuple[tuple[str, object], ...]
    old_fact: str
    new_fact: str
    old_occurrence: str | None
    new_occurrence: str | None
    sibling_key: str
    sibling_fact: str


@dataclass(frozen=True)
class Program:
    motif: str
    seed: int
    originals: tuple[Original, ...]
    initial: tuple[tuple[str, object], ...]
    steps: tuple[Step, ...]
    checked: tuple[Witness, ...]


def field_key(root: str, field: int) -> str:
    return f"field:{root}:{field}"


def root_key(root: str) -> str:
    return f"root:{root}"


def stable_digest(program: Program) -> str:
    return hashlib.sha256(json.dumps(asdict(program), sort_keys=True,
                                     separators=(",", ":")).encode()).hexdigest()
