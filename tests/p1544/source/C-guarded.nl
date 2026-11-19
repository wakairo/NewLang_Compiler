fn bounded(n: usize) -> usize {
    if (n < usize(10)) {
        let next = n + usize(1);
        next
    } else {
        n
    }
}
fn main() -> usize {
    let q = bounded(usize(50));
    q
}
