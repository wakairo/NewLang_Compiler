struct Dial { gain: u8, flag: u8 }
fn revise_bit(cell: ref<write,u8>, proposed: u8) -> u8 {
    replace(cell, proposed)
}
fn main() -> unit {
    let indicator = Dial { flag: u8(43), gain: u8(12) };
    let baseline = indicator@gain;
    if (true) {
        let old_flag = loan_write(indicator@flag) { |cell| revise_bit(cell, u8(87)) };
        let new_flag = indicator@flag;
        let untouched = indicator@gain;
        old_flag;
        new_flag;
        untouched;
        unit
    } else {
        return unit;
    };
    let after_flag = indicator@flag;
    let after_gain = indicator@gain;
    baseline;
    after_flag;
    after_gain;
    unit
}
