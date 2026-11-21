fn route(load: usize, reserve: usize) -> usize {
    let groups = load / usize(6);
    let spare = reserve % usize(9);
    let selected = if (groups < usize(10)) {
        spare
    } else {
        groups
    };
    let copied = selected;
    copied
}

fn main() -> unit {
    let trip = route(usize(43), usize(29));
    let remote = route(usize(83), usize(17));
    let residue = remote % usize(5);
    return unit;
}
