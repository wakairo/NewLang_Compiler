fn main() -> unit {
    let selector = usize(1) < usize(2);
    let seed = u8(17);
    let picked = if (selector) {
        let left = seed;
        left
    } else {
        let right = u8(91);
        right
    };
}
