fn main() -> unit {
    let selector = true;
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
