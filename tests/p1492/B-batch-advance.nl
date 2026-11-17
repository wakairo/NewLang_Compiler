fn batch_advance(count: usize) -> usize {
    if (count < usize(9)) {
        let advanced = count + usize(7);
        advanced
    } else {
        count
    }
}

fn main() -> usize {
    let advanced = batch_advance(usize(8));
    let snapshot = advanced;
    snapshot
}
