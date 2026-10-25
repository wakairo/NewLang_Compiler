struct QuartzLedger { pulse: u8, reserve: u8 }

fn remap_quartz(slot: ref<write,u8>, packet: u8) -> u8 {
    let displaced = replace(slot, packet);
    return displaced;
}

fn main() -> u8 {
    let active = QuartzLedger { reserve: u8(43), pulse: u8(181) };
    let standby = QuartzLedger { pulse: u8(29), reserve: u8(66) };
    if (usize(12) == usize(12)) {
        loan_write(active@pulse) { |write_pulse| remap_quartz(write_pulse, u8(57)) }
    } else {
        loan_write(active@pulse) { |write_pulse_else| remap_quartz(write_pulse_else, u8(96)) }
    };
    let current = active@pulse;
    let untouched = active@reserve;
    let other_root = standby@pulse;
    return current;
}
