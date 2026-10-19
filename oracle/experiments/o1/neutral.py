"""O1 non-normative, synthetic SOURCE operations and untrusted event evidence.
Only vocabulary and deterministic identifiers live here; NO checking/transition code.
"""
from dataclasses import dataclass, asdict
import hashlib
import json


@dataclass(frozen=True)
class Pointer:
    root: str
    region: str
    incarnation: str


@dataclass(frozen=True)
class Option:
    payload: Pointer | None = None


@dataclass(frozen=True)
class Original:
    root: str
    region: str
    incarnation: str
    allocation: str
    domain: str
    nominal: str = "Handle"
    field_count: int = 3


@dataclass(frozen=True)
class Claim:
    value: Option
    fact: str
    occurrence: str | None


@dataclass(frozen=True)
class OpenLoan:
    node: str
    loan: str
    root: str
    field: int
    domain: str
    mode: str = "write"


@dataclass(frozen=True)
class Change:
    node: str
    loan: str
    field: int
    value: Option
    alias: str = "known"


@dataclass(frozen=True)
class CloseLoan:
    node: str
    loan: str


@dataclass(frozen=True)
class HoldPayload:
    node: str
    hold: str
    root: str
    field: int
    occurrence: str


@dataclass(frozen=True)
class DropPayload:
    node: str
    hold: str


@dataclass(frozen=True)
class Unmodeled:
    node: str
    operation: str


Operation = OpenLoan | Change | CloseLoan | HoldPayload | DropPayload | Unmodeled


@dataclass(frozen=True)
class Evidence:
    """A snapshot *claim*, never a root/loan/owner grant."""
    ordinal: int
    node: str
    kind: str
    before: Claim | None = None
    after: Claim | None = None
    siblings_before: tuple[tuple[int, Claim], ...] = ()
    siblings_after: tuple[tuple[int, Claim], ...] = ()


@dataclass(frozen=True)
class Program:
    originals: tuple[Original, ...]
    operations: tuple[Operation, ...]
    evidence: tuple[Evidence, ...]
    motif: str


@dataclass(frozen=True)
class Outcome:
    decision: str
    first_event: str | None = None
    rule: str = "OK"

    @property
    def supported(self) -> bool:
        return self.decision != "UNSUPPORTED"


def digest(program: Program) -> str:
    return hashlib.sha256(json.dumps(asdict(program), sort_keys=True,
                                     separators=(",", ":")).encode()).hexdigest()
