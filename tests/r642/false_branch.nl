fn main() -> unit {
    let selector = usize(7) < usize(2);
    let seed = u8(41);
    let picked = if (selector) {
        let left = seed;
        left
    } else {
        let right = u8(52);
        right
    };
    let second = u8(14);
    let after = second;
}
