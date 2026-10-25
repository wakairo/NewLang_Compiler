struct Meridian {
    cobalt: u8,
    amber: u8,
}

fn recover_before(cell: ref<write,u8>, offered: u8) -> u8 {
    let displaced = replace(cell, offered);
    return offered;
}

fn main() -> unit {
    let gauge = Meridian{amber: u8(163), cobalt: u8(37)};
    let remote = Meridian{cobalt: u8(59), amber: u8(201)};
    if (u8(9) == u8(9)) {
        loan_write(gauge@cobalt) { |access|
            recover_before(access, u8(81))
        }
    } else {
        loan_write(gauge@cobalt) { |access|
            recover_before(access, u8(118))
        }
    };
    let current = gauge@cobalt;
    let untouched = gauge@amber;
    let elsewhere = remote@cobalt;
    unit
}
