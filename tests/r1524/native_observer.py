"""R1524 observation of our own emitted C; no guest-state construction."""

import re


OBSERVER = r"""
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
    unsigned fail_at, payload, attempts, allocated, released;
    unsigned copies, stores, returns, receiver_entries, receiver_leaves;
    unsigned h_reads, t_finished, live_payloads_seen;
    int copy_active, receiver_active;
    size_t h_size, h_offset;
    void *live[2];
    const void *t_object;
    const uint8_t *t0, *t1;
    unsigned old[2], new_value[2], free_order[2], free_payload[2];
} R1524State;

static R1524State r1524;

static void r1524_require(int ok, const char *why)
{
    if (!ok) {
        fprintf(stderr, "R1524 NATIVE_ASSERT: %s\n", why);
        exit(91);
    }
}

static void r1524_mapping(const char *why)
{
    fprintf(stderr, "R1524 HARNESS-MAPPING: %s\n", why);
    exit(92);
}

static void r1524_init(unsigned fail_at, unsigned payload, size_t h_size,
                       size_t h_offset)
{
    r1524.fail_at = fail_at;
    r1524.payload = payload;
    r1524.h_size = h_size;
    r1524.h_offset = h_offset;
    r1524_require(h_offset < h_size, "H payload offset outside H");
}

static void r1524_bind_t(const void *object, const uint8_t *a,
                         const uint8_t *b)
{
    r1524_require(r1524.t_object == NULL, "more than one original T");
    r1524_require(object != NULL && a != NULL && b != NULL && a != b,
                   "T original/children missing or aliased");
    r1524.t_object = object;
    r1524.t0 = a;
    r1524.t1 = b;
    /* Addresses only: declaration need not yet have initialized fields. */
}

static void r1524_copy_enter(void)
{
    r1524_require(!r1524.copy_active && r1524.t0 != NULL,
                   "Copy call overlap or missing live original T");
    r1524_require(r1524.copies < 2, "extra named Copy call");
    ++r1524.copies;
    r1524.copy_active = 1;
    if (r1524.copies == 1) {
        r1524_require(r1524.attempts == 0, "first T write moved after H");
    } else {
        r1524_require(r1524.fail_at == 0 && r1524.released == 2 &&
                           r1524.receiver_entries == 1 &&
                           r1524.receiver_leaves == 1 &&
                           !r1524.receiver_active,
                       "second T write before complete H2 cleanup");
    }
}

static void r1524_before_store(const uint8_t *target)
{
    unsigned expected = r1524.stores == 0 ? 29u : 149u;
    r1524_require(r1524.copy_active && r1524.stores < 2,
                   "store outside selected named Copy");
    r1524_require(target == r1524.t0 && r1524.t1 != NULL,
                   "store changed original T or field");
    r1524_require(*target == expected && *r1524.t1 == 101,
                   "wrong actual old T value or disjoint sibling");
    r1524.old[r1524.stores] = *target;
}

static void r1524_after_store(const uint8_t *target)
{
    unsigned expected = r1524.stores == 0 ? 149u : r1524.payload;
    r1524_require(target == r1524.t0 && r1524.stores < 2,
                   "post-store target identity");
    r1524_require(*target == expected && *r1524.t1 == 101,
                   "wrong actual updated T value or sibling");
    r1524.new_value[r1524.stores] = *target;
    ++r1524.stores;
}

static void r1524_copy_leave(uint8_t actual_old)
{
    r1524_require(r1524.copy_active && r1524.returns < r1524.stores,
                   "named Copy return ordering");
    r1524_require(actual_old == r1524.old[r1524.returns],
                   "named Copy returned a fabricated old value");
    ++r1524.returns;
    r1524.copy_active = 0;
}

static void *r1524_malloc(size_t size)
{
    void *p;
    r1524_require(size == r1524.h_size, "unexpected guest allocation shape");
    r1524_require(r1524.attempts < 2, "third allocation attempt");
    ++r1524.attempts;
    if (r1524.fail_at == r1524.attempts)
        return NULL;
    p = malloc(size);
    r1524_require(p != NULL, "unexpected target allocator failure");
    r1524_require(r1524.allocated < 2, "third physical allocation");
    if (r1524.allocated == 1)
        r1524_require(p != r1524.live[0],
                       "simultaneously live original regions overlap");
    r1524.live[r1524.allocated] = p;
    ++r1524.allocated;
    return p;
}

static void r1524_note_load(const uint8_t *address, unsigned actual)
{
    if (r1524.live[1] != NULL) {
        const unsigned char *p = r1524.live[1];
        if ((const void *)address ==
            (const void *)(p + r1524.h_offset)) {
            r1524_require(actual == r1524.payload &&
                               actual == p[r1524.h_offset],
                           "actual H2 load does not match live payload");
            r1524_require(r1524.released == 0 &&
                               !r1524.receiver_active,
                           "H2 load after lifetime release boundary");
            ++r1524.h_reads;
        }
    }
}

static void r1524_receiver_enter(const void *p, const void *allocation)
{
    const unsigned char *first;
    const unsigned char *second;
    r1524_require(!r1524.receiver_active && r1524.receiver_entries == 0,
                   "extra/overlapping receiver");
    r1524_require(r1524.allocated == 2 && r1524.released == 0 &&
                           r1524.live[0] != NULL &&
                           r1524.live[1] != NULL,
                   "receiver reached without both live originals");
    r1524_require(p == r1524.live[1] &&
                           allocation == r1524.live[1],
                   "receiver actual p/Allocation is not second original");
    first = r1524.live[0];
    second = r1524.live[1];
    /* Entry is before the receiver's EndRoot; both originals are live. */
    r1524_require(first[r1524.h_offset] == 29 &&
                           second[r1524.h_offset] == r1524.payload,
                   "actual live original H payloads at receiver entry");
    r1524.live_payloads_seen = 2;
    r1524.receiver_active = 1;
    ++r1524.receiver_entries;
}

static void r1524_receiver_leave(void)
{
    r1524_require(r1524.receiver_active && r1524.released == 1 &&
                           r1524.free_order[0] == 2 &&
                           r1524.live[1] == NULL &&
                           r1524.live[0] != NULL,
                   "callee did not physically free only second original");
    r1524.receiver_active = 0;
    ++r1524.receiver_leaves;
}

static void r1524_free(void *p)
{
    unsigned id;
    unsigned datum;
    r1524_require(p != NULL, "phantom/null guest free");
    if (p == r1524.live[0])
        id = 1;
    else if (p == r1524.live[1])
        id = 2;
    else {
        r1524_require(0, "foreign or duplicate physical free");
        return;
    }
    r1524_require(r1524.released < 2, "extra physical free");
    /* Raw still-allocated byte observation only: EndRoot has occurred.
     * No payload equality or typed-H liveness is inferred from these bytes. */
    datum = ((const unsigned char *)p)[r1524.h_offset];
    if (id == 2) {
        r1524_require(r1524.receiver_active && r1524.released == 0,
                       "second original not freed first inside callee");
    } else {
        r1524_require(!r1524.receiver_active,
                       "first original freed in second receiver");
        r1524_require(r1524.fail_at == 2 ||
                               (r1524.fail_at == 0 &&
                                r1524.released == 1 &&
                                r1524.receiver_leaves == 1),
                       "caller first-free order/world");
    }
    r1524.free_order[r1524.released] = id;
    r1524.free_payload[r1524.released] = datum;
    ++r1524.released;
    r1524.live[id - 1] = NULL;
    /* The one genuine physical free. No pointer is read after this call. */
    free(p);
}

static void r1524_finish_t(void)
{
    unsigned expected = r1524.fail_at == 0 ? r1524.payload : 149u;
    r1524_require(!r1524.t_finished && r1524.t0 != NULL &&
                           r1524.t1 != NULL,
                   "T final observation outside its original lifetime");
    r1524_require(*r1524.t0 == expected && *r1524.t1 == 101,
                   "post-join actual current/sibling values");
    r1524.t_finished = 1;
    r1524.t0 = NULL;
    r1524.t1 = NULL;
    r1524.t_object = NULL;
}

static void r1524_finish(void)
{
    unsigned successes = r1524.fail_at == 1 ? 0u :
                         r1524.fail_at == 2 ? 1u : 2u;
    unsigned calls = successes == 2 ? 2u : 1u;
    unsigned receivers = successes == 2 ? 1u : 0u;
    unsigned attempts = r1524.fail_at == 1 ? 1u : 2u;
    r1524_require(r1524.attempts == attempts &&
                           r1524.allocated == successes &&
                           r1524.released == successes,
                   "H0/H1/H2 physical allocation/free counts");
    r1524_require(r1524.copies == calls && r1524.stores == calls &&
                           r1524.returns == calls && !r1524.copy_active,
                   "H0/H1/H2 actual T call/store/old-return counts");
    r1524_require(r1524.receiver_entries == receivers &&
                           r1524.receiver_leaves == receivers &&
                           !r1524.receiver_active,
                   "H0/H1/H2 receiver entry/return counts");
    r1524_require(r1524.t_finished && r1524.live[0] == NULL &&
                           r1524.live[1] == NULL,
                   "unobserved T exit or live allocation leak");
    if (successes == 2 && r1524.h_reads == 0)
        r1524_mapping("no recognized actual H scalar-load tap");
    r1524_require(r1524.h_reads == receivers,
                   "unexpected actual H scalar-load count");
    r1524_require(r1524.live_payloads_seen == (successes == 2 ? 2u : 0u),
                   "live H payload observation count");
    printf("R1524 WORLD H%u attempts=%u allocated=%u freed=%u "
           "receiver=%u writes=%u old0=%u old1=%u "
           "current=%u sibling=101 payload=%u "
           "live_h_payloads_seen=%u raw_prefree0=%u raw_prefree1=%u\n",
           successes, r1524.attempts, r1524.allocated, r1524.released,
           r1524.receiver_entries, r1524.stores, r1524.old[0],
           r1524.old[1], successes == 2 ? r1524.payload : 149u,
           r1524.payload, r1524.live_payloads_seen,
           r1524.free_payload[0], r1524.free_payload[1]);
    if (successes == 1)
        puts("R1524 H1 payload: raw pre-free bytes only; "
             "live typed H payload read NOT OBSERVED");
    puts("R1524 NATIVE_PASS");
}
"""


def _fail(why):
    raise ValueError("R1524 HARNESS-MAPPING: " + why)


def _clean(text):
    pattern = r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\''
    return re.sub(
        pattern,
        lambda m: "".join("\n" if c == "\n" else " " for c in m[0]),
        text,
    )


def _close_brace(clean, opening):
    depth = 0
    for i in range(opening, len(clean)):
        if clean[i] == "{":
            depth += 1
        elif clean[i] == "}":
            depth -= 1
            if depth == 0:
                return i
    _fail("unbalanced generated function")


def _parameter(text):
    m = re.fullmatch(r"\s*(.*?)([A-Za-z_]\w*)\s*", text)
    if not m:
        _fail("unrecognized parameter")
    prefix, name = m.groups()
    if not prefix.strip() or prefix.count("*") > 1:
        _fail("unrecognized parameter type")
    pointer = "*" in prefix
    base = re.sub(r"\bconst\b", "", prefix.replace("*", ""))
    base = " ".join(base.split())
    if not re.fullmatch(
        r"(?:(?:struct )?[A-Za-z_]\w*|unsigned char)", base
    ):
        _fail("unrecognized parameter base type")
    return {"type": base, "pointer": pointer, "name": name}


def _fields(body):
    fields = []
    for item in body.split(";"):
        if not item.strip():
            continue
        scalar_names = re.fullmatch(
            r"\s*uint8_t\s+"
            r"([A-Za-z_]\w*(?:\s*,\s*[A-Za-z_]\w*)+)\s*",
            item,
        )
        if scalar_names:
            for name in scalar_names[1].split(","):
                fields.append(_parameter("uint8_t " + name.strip()))
        else:
            fields.append(_parameter(item))
    names = [field["name"] for field in fields]
    if len(names) != len(set(names)):
        _fail("duplicate generated struct field")
    return fields


def _statement_end(clean, start):
    depth = 0
    for i in range(start, len(clean)):
        c = clean[i]
        if c in "({[":
            depth += 1
        elif c in ")}]":
            depth -= 1
        elif c == ";" and depth == 0:
            return i + 1
    _fail("unterminated generated declaration")


def instrument(generated: str) -> str:
    clean = _clean(generated)
    if re.search(r"\bvolatile\b", clean):
        _fail("volatile generated C is outside this observer")

    structs = {}
    records = []
    typedef = re.compile(
        r"\btypedef\s+struct(?:\s+([A-Za-z_]\w*))?\s*"
        r"\{([^{}]*)\}\s*([A-Za-z_]\w*)\s*;"
    )
    for m in typedef.finditer(clean):
        tag, body, alias = m.groups()
        record = {"type": alias, "fields": _fields(body)}
        records.append(record)
        structs[alias] = record
        if tag:
            structs["struct " + tag] = record
    tagged = re.compile(r"\bstruct\s+([A-Za-z_]\w*)\s*\{([^{}]*)\}\s*;")
    for m in tagged.finditer(clean):
        tag, body = m.groups()
        key = "struct " + tag
        if key not in structs:
            record = {"type": key, "fields": _fields(body)}
            records.append(record)
            structs[key] = record
    for m in re.finditer(
        r"\btypedef\s+struct\s+(\w+)\s+(\w+)\s*;", clean
    ):
        tag, alias = m.groups()
        if "struct " + tag in structs:
            structs[alias] = structs["struct " + tag]

    funcs = []
    pattern = re.compile(
        r"\b(uint8_t|void|int)\s+([A-Za-z_]\w*)\s*"
        r"\(([^()]*)\)\s*\{"
    )
    previous_end = -1
    for m in pattern.finditer(clean):
        if m.start() < previous_end:
            continue
        opening = clean.find("{", m.start(), m.end())
        closing = _close_brace(clean, opening)
        raw = m[3].strip()
        params = [] if raw in ("", "void") else [
            _parameter(x) for x in raw.split(",")
        ]
        funcs.append({
            "ret": m[1], "name": m[2], "params": params,
            "open": opening, "close": closing,
            "body": clean[opening + 1:closing],
        })
        previous_end = closing + 1

    copies = [
        f for f in funcs
        if f["ret"] == "uint8_t" and len(f["params"]) == 2
        and f["params"][0]["type"] == "uint8_t"
        and f["params"][0]["pointer"]
        and f["params"][1]["type"] == "uint8_t"
        and not f["params"][1]["pointer"]
    ]
    if len(copies) != 1:
        _fail("expected one real uint8_t write/old-Copy helper")
    copy = copies[0]

    receivers = []
    for f in funcs:
        if f["ret"] != "void" or len(f["params"]) != 3:
            continue
        p = f["params"][0]
        if not p["pointer"] or p["type"] not in structs:
            continue
        h = structs[p["type"]]
        scalar = [
            x for x in h["fields"]
            if x["type"] == "uint8_t" and not x["pointer"]
        ]
        if len(scalar) == 1 and re.search(r"\bfree\s*\(", f["body"]):
            receivers.append((f, h, scalar[0]["name"]))
    if len(receivers) != 1:
        _fail("expected one separate three-parameter terminal receiver")
    receiver, h, h_field = receivers[0]

    ts = [
        x for x in records
        if len(x["fields"]) == 2
        and all(y["type"] == "uint8_t" and not y["pointer"]
                for y in x["fields"])
    ]
    if len(ts) != 1:
        _fail("expected one exact two-u8 T layout")
    t = ts[0]

    mains = [
        f for f in funcs
        if f["name"] == "main" and f["ret"] == "int"
        and not f["params"]
    ]
    if len(mains) != 1:
        _fail("expected int main(void)")

    edits = []

    def insert(at, text):
        edits.append((at, at, "\n" + text + "\n"))

    # Actual T addresses only; no initialization or value substitution.
    declarations = []
    t_types = [name for name, record in structs.items() if record is t]
    for f in funcs:
        for type_name in t_types:
            spelling = r"\s+".join(map(re.escape, type_name.split()))
            decl = re.compile(
                r"(?<!\w)" + spelling +
                r"\s+([A-Za-z_]\w*)\s*(?==|;)"
            )
            for m in decl.finditer(f["body"]):
                start = f["open"] + 1 + m.start()
                prefix = clean[f["open"] + 1:start]
                if prefix.count("{") != prefix.count("}"):
                    _fail("T local is not in its function's outer scope")
                end = _statement_end(clean, start)
                declarations.append((f, m[1], end))
    unique = {(f["name"], name, end): (f, name, end)
              for f, name, end in declarations}
    a, b = [x["name"] for x in t["fields"]]
    # Aggregate value temporaries are not the mutable source T object.
    # Bind only the unique outer-scope object passed to both real writes.
    call_name = r"\b" + re.escape(copy["name"]) + r"\s*\("
    field_call = re.compile(
        call_name + r"\s*&\s*([A-Za-z_]\w*)\s*\.\s*" +
        re.escape(a) + r"\s*,"
    )
    calls = [(f["name"], m[1]) for f in funcs
             for m in field_call.finditer(f["body"])]
    all_calls = sum(len(re.findall(call_name, f["body"])) for f in funcs)
    if len(calls) != 2 or all_calls != 2 or len(set(calls)) != 1:
        _fail("expected two real writes to the same original T field")
    selected = [(f, name, end) for f, name, end in unique.values()
                if (f["name"], name) == calls[0]]
    if len(selected) != 1:
        _fail("expected one outer-scope T declaration for actual writes")
    t_function, t_name, t_end = selected[0]
    insert(t_end,
           f"r1524_bind_t(&{t_name}, &{t_name}.{a}, &{t_name}.{b});")

    # The named Copy store and return each evaluate the guest expression once.
    target = copy["params"][0]["name"]
    store_pattern = re.compile(
        r"(?m)(?:^|(?<=;))[ \t]*(?:\*\s*" + re.escape(target) +
        r"|\(\s*\*\s*" + re.escape(target) +
        r"\s*\))\s*=(?!=)[^;{}]+;"
    )
    stores = list(store_pattern.finditer(copy["body"]))
    if len(stores) != 1:
        _fail("expected exactly one simple indirect store in Copy helper")
    s = stores[0]
    begin = copy["open"] + 1 + s.start()
    end = copy["open"] + 1 + s.end()
    insert(begin, f"r1524_before_store({target});")
    insert(end, f"r1524_after_store({target});")
    insert(copy["open"] + 1, "r1524_copy_enter();")
    returns = list(re.finditer(r"\breturn\s+([^;{}]+);", copy["body"]))
    if len(returns) != 1:
        _fail("expected one explicit old-Copy return")
    r = returns[0]
    begin = copy["open"] + 1 + r.start()
    end = copy["open"] + 1 + r.end()
    actual = generated[
        copy["open"] + 1 + r.start(1):
        copy["open"] + 1 + r.end(1)
    ]
    edits.append((
        begin, end,
        "{ uint8_t r1524_return_value = (" + actual + "); "
        "r1524_copy_leave(r1524_return_value); "
        "return r1524_return_value; }"
    ))

    # Observe the actual p and Allocation backing carrier at callee entry.
    p, allocation = receiver["params"][:2]
    if allocation["pointer"]:
        allocation_expr = allocation["name"]
    elif allocation["type"] in structs:
        pointer_fields = [
            x for x in structs[allocation["type"]]["fields"]
            if x["pointer"]
        ]
        if len(pointer_fields) != 1:
            _fail("Allocation carrier has no unique observable backing pointer")
        allocation_expr = (
            allocation["name"] + "." + pointer_fields[0]["name"]
        )
    else:
        _fail("unrecognized native Allocation carrier")
    insert(receiver["open"] + 1,
           "r1524_receiver_enter((const void *)" + p["name"] +
           ", (const void *)" + allocation_expr + ");")
    insert(receiver["close"], "r1524_receiver_leave();")
    for m in re.finditer(r"\breturn\b([^;]*);", receiver["body"]):
        if m[1].strip():
            _fail("terminal void receiver has a value return")
        insert(receiver["open"] + 1 + m.start(),
               "r1524_receiver_leave();")

    # Tap real simple loads only. Nothing is substituted into their result.
    potential_h_loads = 0
    for f in funcs:
        for m in re.finditer(
            r"\buint8_t\s+([A-Za-z_]\w*)\s*=\s*([^;{}]+);",
            f["body"]
        ):
            value, rhs = m.groups()
            rhs = rhs.strip()
            if rhs.startswith("(") and rhs.endswith(")"):
                rhs = rhs[1:-1].strip()
            indirect = re.fullmatch(r"\*\s*([A-Za-z_]\w*)", rhs)
            field = re.fullmatch(
                r"([A-Za-z_]\w*)\s*->\s*" + re.escape(h_field), rhs
            )
            if indirect:
                address = indirect[1]
            elif field:
                address = "&" + field[1] + "->" + h_field
            else:
                continue
            insert(f["open"] + 1 + m.end(),
                   f"r1524_note_load({address}, (unsigned){value});")
            if f is not copy:
                potential_h_loads += 1
    if potential_h_loads == 0:
        _fail("no recognizable actual H scalar-load expression")

    # Read T before its containing function's scope ends, never afterward.
    insert(t_function["close"], "r1524_finish_t();")
    for m in re.finditer(r"\breturn\b([^;]*);", t_function["body"]):
        expression = m[1].strip()
        if expression not in ("", "0", "EXIT_SUCCESS"):
            _fail("T-owning function return needs an explicit safe mapping")
        insert(t_function["open"] + 1 + m.start(), "r1524_finish_t();")

    # Reject overlapping replacements rather than corrupt guest expressions.
    replacements = sorted((a, b) for a, b, _ in edits if a != b)
    for left, right in zip(replacements, replacements[1:]):
        if left[1] > right[0]:
            _fail("overlapping observation replacements")
    for point, end, _ in edits:
        if point == end and any(a < point < b for a, b in replacements):
            _fail("observation insertion inside another replacement")
    result = generated
    for begin, end, text in sorted(edits, reverse=True):
        result = result[:begin] + text + result[end:]

    driver = r"""
#undef malloc
#undef free
#undef main

int main(int argc, char **argv)
{
    char *end = NULL;
    unsigned long failure, payload;
    int guest_result;
    if (argc != 3)
        return 90;
    failure = strtoul(argv[1], &end, 10);
    if (*end != '\0' || failure > 2)
        return 90;
    payload = strtoul(argv[2], &end, 10);
    if (*end != '\0' || (payload != 0 && payload != 211))
        return 90;
    r1524_init((unsigned)failure, (unsigned)payload,
                sizeof(R1524_H_TYPE),
                offsetof(R1524_H_TYPE, R1524_H_FIELD));
    guest_result = r1524_guest_main();
    r1524_require(guest_result == 0, "generated guest main failed");
    r1524_finish();
    return 0;
}
"""
    driver = driver.replace("R1524_H_TYPE", h["type"])
    driver = driver.replace("R1524_H_FIELD", h_field)
    macros = (
        "\n#define malloc r1524_malloc\n"
        "#define free r1524_free\n"
        "#define main r1524_guest_main\n"
    )
    return OBSERVER + macros + result + driver
