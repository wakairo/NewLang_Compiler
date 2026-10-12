"""Independent deterministic synthetic cases, not a NewLang source parser."""
from dataclasses import replace
import random
from neutral import (Change, Claim, CloseLoan, Evidence, OpenLoan, Option,
                     Original, Pointer, Program)

LENGTHS = (2, 8, 9, 10, 12)


def make(motif: str, count: int, seed: int = 20261012) -> Program:
    if motif not in ("device_directory", "intrusive_scheduler") or count < 1:
        raise ValueError((motif, count))
    if motif == "device_directory":
        originals = (Original("directory", "br_dir", "inc_dir", "alloc_dir", "d_dir"),
                     Original("slot2", "br_s2", "inc_s2", "alloc_s2", "d_s2"),
                     Original("slot4", "br_s4", "inc_s4", "alloc_s4", "d_s4"))
    else:
        originals = (Original("schedulerX", "br_x", "inc_x", "alloc_x", "d_x"),
                     Original("schedulerY", "br_y", "inc_y", "alloc_y", "d_y"))
    lookup = {x.root: x for x in originals}
    current = {(o.root, k): Claim(Option(), f"initial:{o.root}:{k}", None)
               for o in originals for k in range(3)}
    instructions, evidence = [], []
    randomizer = random.Random(seed)

    def append(op, before=None, after=None, sb=(), sa=()):
        instructions.append(op)
        evidence.append(Evidence(len(evidence), op.node, type(op).__name__, before, after, sb, sa))

    for n in range(count):
        if motif == "device_directory":
            if n < 4:
                root, f = "directory", 2
                target = ("slot2", None, "slot4", None)[n]
            else:
                root, f = (("directory", 0), ("slot2", 0), ("directory", 1),
                           ("slot4", 0))[((n - 4) % 4)]
                target = ("slot4", "directory", None, "slot2")[(n - 4) % 4]
        else:
            choices = (("schedulerX", 0, "schedulerY"),
                       ("schedulerY", 1, "schedulerX"),
                       ("schedulerX", 0, "schedulerX"),
                       ("schedulerY", 1, None),
                       ("schedulerX", 0, "schedulerX"),  # Some(same) -> Some(same)
                       ("schedulerY", 1, "schedulerY"))
            root, f, target = choices[n % len(choices)]
        # Seed changes node naming, not the expected state transition algorithm.
        suffix = f"{n + 1:02d}_{randomizer.getrandbits(32):08x}"
        loan, open_node, change_node, close_node = (f"loan:{suffix}", f"open:{suffix}",
                                                     f"change:{suffix}", f"close:{suffix}")
        new = Option(Pointer(target, lookup[target].region, lookup[target].incarnation) if target else None)
        append(OpenLoan(open_node, loan, root, f, lookup[root].domain))
        old = current[root, f]
        sibling_before = tuple((k, current[root, k]) for k in range(3) if k != f)
        updated = Claim(new, f"fact:{change_node}", f"occ:{change_node}" if target else None)
        current[root, f] = updated
        sibling_after = tuple((k, current[root, k]) for k in range(3) if k != f)
        append(Change(change_node, loan, f, new), old, updated, sibling_before, sibling_after)
        append(CloseLoan(close_node, loan))
    return Program(originals, tuple(instructions), tuple(evidence), motif)


def alter_op(program: Program, position: int, **updates) -> Program:
    sequence = list(program.operations)
    sequence[position] = replace(sequence[position], **updates)
    return replace(program, operations=tuple(sequence))


def alter_evidence(program: Program, position: int, **updates) -> Program:
    proof = list(program.evidence)
    proof[position] = replace(proof[position], **updates)
    return replace(program, evidence=tuple(proof))


def paired_mutations(program: Program) -> dict[str, Program]:
    """Proof corruption is NOT a way to grant source authority."""
    if len(program.operations) < 27:
        raise ValueError("use >=9 Changes for ninth-event poisoning")
    ninth_open, ninth_change = 8 * 3, 8 * 3 + 1
    cert = program.evidence[ninth_change]
    old = cert.before
    new = cert.after
    op = program.operations[ninth_open]
    assert isinstance(op, OpenLoan) and old is not None and new is not None
    items = {}
    items["wrong_original_domain"] = alter_op(program, ninth_open, domain="wrong_D_copy_ptr_cannot_grant")
    items["read_to_write_amplification"] = alter_op(program, ninth_open, mode="read")
    items["stale_old_occurrence"] = alter_evidence(
        program, ninth_change, before=replace(old, occurrence="retired_occurrence"))
    items["forged_new_occurrence"] = alter_evidence(
        program, ninth_change, after=replace(new, occurrence="forged_occurrence"))
    items["lost_ninth_proof"] = replace(program, evidence=tuple(x for i, x in enumerate(program.evidence)
                                                        if i != ninth_change))
    items["duplicated_ninth_proof"] = replace(program, evidence=(program.evidence[:ninth_change] +
                                                 (cert, cert) + program.evidence[ninth_change + 1:]))
    proofs = list(program.evidence)
    proofs[ninth_change - 1], proofs[ninth_change] = proofs[ninth_change], proofs[ninth_change - 1]
    items["reordered_ninth_proof"] = replace(program, evidence=tuple(proofs))
    items["wrong_source_node"] = alter_evidence(program, ninth_change, node="fabricated:node9")
    first_sibling = cert.siblings_before[0]
    bogus = (first_sibling[0], replace(first_sibling[1], fact="not_current"))
    items["sibling_currentness_forged"] = alter_evidence(
        program, ninth_change, siblings_before=(bogus,) + cert.siblings_before[1:])
    items["missing_write_loan"] = alter_op(program, ninth_change, loan="arbitrary_ptr_loan")
    pointer = next((o.value.payload for o in program.operations if isinstance(o, Change)
                    and o.value.payload is not None), None)
    if pointer is not None:
        items["ptr_token_as_option_value"] = alter_op(program, ninth_change, value=pointer)
        items["invalid_ptr_provenance"] = alter_op(program, ninth_change,
                                                  value=Option(replace(pointer, region="forged_alias_region")))
    items["unknown_alias"] = alter_op(program, ninth_change, alias="unknown")
    return items
