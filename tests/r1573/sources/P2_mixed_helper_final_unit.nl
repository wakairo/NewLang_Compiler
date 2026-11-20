fn main() -> unit {
    let pack = usize(86);
    let direct = pack / usize(4);
    let pre = direct;
    let first = carve(pre);
    let second = carve(pack);
    let residue = second % usize(6);
    let back = residue + first;
    let final = back - usize(2);
    let copied = final;
    return unit;
}

fn carve(quantity: usize) -> usize {
    let before = quantity;
    let segment = before / usize(4);
    let after = segment;
    return after;
}
