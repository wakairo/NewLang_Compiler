fn main() -> usize {
    loop (outer = usize(1)) {
        loop (inner = usize(2), mark = usize(0)) {
            continue(outer);
        };
        break outer;
    }
}
