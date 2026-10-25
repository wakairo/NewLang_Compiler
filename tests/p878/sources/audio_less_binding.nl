struct AudioCalibration { identity:u8, gain:u8 }
fn apply_setting(write_ref:ref<write,u8>,new_setting:u8)->unit {
    if (usize(34) < usize(52)) {
        return unit;
    } else {
        let displaced = replace(write_ref,new_setting);
        unit
    }
}
fn main()->unit {
    let settings = AudioCalibration{identity:u8(206),gain:u8(44)};
    let sink = AudioCalibration{identity:u8(119),gain:u8(13)};
    let before = settings@gain;
    let completed = loan_write(settings@gain){|active|apply_setting(active,u8(87))};
    let current = settings@gain;
    let sibling = settings@identity;
    let sink_before = loan_write(sink@identity){|active|replace(active,current)};
    let sink_current = sink@identity;
    unit
}
