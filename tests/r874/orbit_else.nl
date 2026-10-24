struct OrbitTag { north: u8, south: u8 }

fn pulse_at_difference(slot: ref<write,u8>, left: usize, right: usize) -> u8 {
    if (left == right) {
        return u8(6);
    } else {
        let from_before = replace(slot, u8(83));
        from_before
    }
}

fn main() -> u8 {
    let leader = OrbitTag { north: u8(31), south: u8(7) };
    let trailer = OrbitTag { north: u8(47), south: u8(13) };
    let original = leader@north;
    let normal = loan_write(leader@north) { |focus|
        pulse_at_difference(focus, usize(4), usize(10))
    };
    let terminated = loan_write(trailer@north) { |other_focus|
        pulse_at_difference(other_focus, usize(4), usize(4))
    };
    let after_normal = leader@north;
    let sibling = leader@south;
    let after_terminated = trailer@north;
    let untouched_sibling = trailer@south;
    original + normal + after_normal + sibling + terminated + after_terminated + untouched_sibling
}
