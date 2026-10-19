fn main() -> unit {
    let selector = usize(6) == usize(6);
    let picked = if (selector) {
        let first = u8(73);
        let left = first;
        left
    } else {
        let second = u8(73);
        let right = second;
        right
    };
}
