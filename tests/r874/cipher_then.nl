struct CipherPad { ember: u8, frost: u8 }

fn pulse_at_equal(target: ref<write,u8>, front: usize, back: usize) -> u8 {
    if (front == back) {
        let previous = replace(target, u8(71));
        previous
    } else {
        return u8(5);
    }
}

fn main() -> u8 {
    let primary = CipherPad { ember: u8(17), frost: u8(29) };
    let secondary = CipherPad { ember: u8(43), frost: u8(11) };
    let original = primary@ember;
    let sibling_original = primary@frost;
    let second_original = secondary@ember;
    let normally = loan_write(primary@ember) { |port|
        pulse_at_equal(port, usize(12), usize(12))
    };
    let terminated = loan_write(secondary@ember) { |another|
        pulse_at_equal(another, usize(12), usize(19))
    };
    let current = primary@ember;
    let sibling_current = primary@frost;
    let second_current = secondary@ember;
    let second_sibling = secondary@frost;
    original + sibling_original + normally + current + sibling_current + terminated + second_current + second_sibling
}
