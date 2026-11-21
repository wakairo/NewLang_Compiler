fn batch(load: usize) -> usize {
    load / usize(6)
}

fn leftover(reserve: usize) -> usize {
    reserve % usize(9)
}

fn main() -> unit {
    let near = batch(usize(43));
    let far = leftover(usize(29));
    let selected = if (near < usize(10)) {
        far
    } else {
        near
    };
    let result = selected;
    let audit = result % usize(5);
    unit
}
