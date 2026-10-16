struct Pair {
    left: u8,
    right: u8,
}

fn echo_nucleus(received: u8) -> u8 {
    received
}

fn main() -> unit {
    let carrier = Pair { right: u8(182), left: u8(61) };
    let lhs = usize(27);
    let rhs = usize(27);
    let selected = if (lhs == rhs) {
        u8(137)
    } else {
        u8(23)
    };
    let forwarded = echo_nucleus(selected);
    let former = loan_write(carrier@left) { |dest|
        replace(dest, forwarded)
    };
    let observed = carrier@left;
    let untouched = carrier@right;
    unit
}
