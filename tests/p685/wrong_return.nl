struct PumpSetting { speed:u8, channel:u8 }
fn main()->unit {
    let pump = PumpSetting { speed:u8(5), channel:u8(83) };
    let supplied = idle_speed();
    let prior = loan_write(pump@speed) { |out| replace(out, supplied) };
    let observed = pump@speed;
    let sibling = pump@channel;
    supplied; prior; observed; sibling; unit
}
fn idle_speed()->u8 { unit }
