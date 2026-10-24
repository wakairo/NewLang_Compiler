struct Gauge { temperature: u8, checksum: u8 }
fn adjust(field: ref<write,u8>, incoming: u8)->unit {
    if (usize(29) == usize(29)) {
        let prior = replace(field,incoming);
        let copied_prior = prior;
        let second_copy = prior;
        unit
    } else {
        return unit;
    }
}
fn main()->unit {
    let original = Gauge{temperature:u8(31),checksum:u8(211)};
    let receiver = Gauge{temperature:u8(147),checksum:u8(8)};
    let sibling_before = original@checksum;
    let invocation = loan_write(original@temperature){|w|adjust(w,u8(72),u8(1))};
    let updated = original@temperature;
    let sibling_after = original@checksum;
    let receiver_old = loan_write(receiver@checksum){|w|replace(w,updated)};
    let receiver_updated = receiver@checksum;
    unit
}
