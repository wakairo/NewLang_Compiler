fn main() -> unit {
    let selector = true;
    let seed = u8(17);
    let picked = if (selector) {
        let left = seed;
        left
    } else {
        let right = u8(91);
        right
    };
}
