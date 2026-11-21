fn main() -> unit {
    let local = through(usize(139));
    let remote = through(usize(857));
    let last = finish(usize(305));
    let residue = remote % usize(5);
    return unit;
}

fn through(count: usize) -> usize {
    let staged = if (count < usize(150)) {
        count
    } else {
        rack(batch(count))
    };
    return overflow(rack(staged));
}

fn finish(count: usize) -> usize {
    rack(overflow(batch(count)))
}

fn rack(units: usize) -> usize {
    units / usize(3)
}

fn overflow(units: usize) -> usize {
    units % usize(13)
}

fn batch(units: usize) -> usize {
    units / usize(6)
}
