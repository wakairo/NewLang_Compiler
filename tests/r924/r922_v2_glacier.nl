struct GlacierMeter {
    quiet: u8,
    spark: u8,
}

fn reset_glacier(target: ref<write,u8>, replacement: u8) -> u8 {
    let earlier = replace(target, replacement);
    return earlier;
}

fn main() -> unit {
    let winter = GlacierMeter { quiet: u8(14), spark: u8(186) };
    let summer = GlacierMeter { quiet: u8(229), spark: u8(75) };
    let remembered = if (usize(2) == usize(7)) {
        loan_write(summer@spark) { |gate|
            reset_glacier(gate, u8(33))
        }
    } else {
        loan_write(summer@spark) { |gate|
            reset_glacier(gate, u8(142))
        }
    };
    unit
}
