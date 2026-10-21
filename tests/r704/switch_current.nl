struct SwitchPanel { mode:u8, reserve:u8 }
fn current_mode()->u8 {
    let internal = SwitchPanel { reserve:u8(215), mode:u8(147) };
    let previous = loan_write(internal@mode) { |writer| replace(writer,u8(22)) };
    let present = internal@mode;
    let sibling = internal@reserve;
    previous; sibling;
    return present;
}
fn main()->unit {
    let receiver = SwitchPanel { reserve:u8(5), mode:u8(64) };
    let fresh = current_mode();
    let displaced = loan_write(receiver@reserve) { |writer| replace(writer,fresh) };
    let observed = receiver@reserve;
    let untouched = receiver@mode;
    displaced; observed; untouched; unit
}
