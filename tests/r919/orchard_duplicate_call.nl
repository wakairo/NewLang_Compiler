struct OrchardCounter { branch: u8, harvest: u8 }

fn settle_harvest(target: ref<write,u8>, arrival: u8) -> u8 {
    let original = replace(target, arrival);
    let historical = original;
    return historical;
}

fn main() -> u8 {
    let inventory = OrchardCounter { harvest: u8(27), branch: u8(90) };
    let offsite = OrchardCounter { branch: u8(155), harvest: u8(68) };
    if (usize(2) == usize(7)) {
        u8(4)
    } else {
        u8(8)
    };
    if (usize(29) == usize(41)) {
        loan_write(inventory@harvest) { |write_then| settle_harvest(write_then, u8(189)) };
        loan_write(inventory@harvest) { |write_again| settle_harvest(write_again, u8(14)) }
    } else {
        loan_write(inventory@harvest) { |write_else| settle_harvest(write_else, u8(53)) }
    };
    let present = inventory@harvest;
    let untouched = inventory@branch;
    let elsewhere = offsite@harvest;
    return present;
}
