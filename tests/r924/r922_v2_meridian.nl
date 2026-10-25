struct MeridianLatch {
    pin: u8,
    reserve: u8,
}

fn lift_meridian(cell: ref<write,u8>, fresh: u8) -> u8 {
    let displaced = replace(cell, fresh);
    return displaced;
}

fn main() -> unit {
    let outer = MeridianLatch { pin: u8(26), reserve: u8(137) };
    let spare = MeridianLatch { pin: u8(79), reserve: u8(204) };
    let remembered = if (usize(12) == usize(12)) {
        loan_write(outer@pin) { |handle|
            lift_meridian(handle, u8(63))
        }
    } else {
        loan_write(outer@pin) { |handle|
            lift_meridian(handle, u8(118))
        }
    };
    unit
}
