fn main() -> unit {
    let total = usize(307);
    let packed = if (usize(8) < usize(12)) {
        let cartons = total / usize(9);
        cartons
    } else {
        let loose = total % usize(17);
        loose
    };
    let held = packed;
    let groups = held / usize(5);
    let residue = held % usize(9);
    let measured = measure(held);
    unit
}

fn measure(value: usize) -> usize {
    let cut = value / usize(3);
    cut
}
