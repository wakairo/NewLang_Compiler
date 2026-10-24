struct Axis { yaw: u8, pitch: u8 }
fn reroute(out: ref<write,u8>, proposed: u8)->unit {
    if (usize(61) == usize(74)) {
        return u8(42);
    } else {
        let previous = replace(out,proposed);
        let snapshot_one = previous;
        let snapshot_two = previous;
        unit
    }
}
fn main()->unit {
    let axis = Axis{yaw:u8(57),pitch:u8(193)};
    let other = Axis{yaw:u8(153),pitch:u8(22)};
    let sibling_before = axis@pitch;
    let invocation = loan_write(axis@yaw){|w|reroute(w,u8(94))};
    let updated = axis@yaw;
    let sibling_after = axis@pitch;
    let previous_other = loan_write(other@yaw){|w|replace(w,updated)};
    let now_other = other@yaw;
    unit
}
