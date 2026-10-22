struct TideGauge {
    port: u8,
    keel: u8,
}

fn gate_keel(dst: ref<write,u8>, flag: u8) -> unit {
    let lhs = usize(8);
    let rhs = usize(5);
    if (lhs < rhs) {
        return unit;
    } else {
        let inner_old = replace(dst, u8(141));
    };
    unit
}

fn main() -> unit {
    let hull = TideGauge { port: u8(23), keel: u8(34) };
    let auxiliary = TideGauge { port: u8(51), keel: u8(62) };
    let old_copy = hull@keel;
    let old_sibling = hull@port;
    loan_write(hull@keel) { |formal_actual|
        gate_keel(formal_actual, u8(7));
    };
    let post_target = hull@keel;
    let post_sibling = hull@port;
    let auxiliary_old = loan_write(auxiliary@port) { |out_ref|
        replace(out_ref, u8(89))
    };
    let auxiliary_post = auxiliary@port;
    let auxiliary_sibling = auxiliary@keel;
    unit
}
