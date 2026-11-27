fn copy_quota(count: usize) -> usize {
    count
}

fn main() -> unit {
    let seed = usize(64);
    let quota = copy_quota(seed);
    let observed = quota;
    return unit;
}
