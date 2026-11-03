struct SampleBank { reserve: u8, pulse: u8 }
fn take_previous(destination: ref<write, u8>, incoming: u8) -> u8 {
    let saved = replace(destination, incoming);
    saved
}
fn main() -> unit {
    let bank = SampleBank{reserve: u8(68), pulse: u8(214)};
    let independent = SampleBank{reserve: u8(13), pulse: u8(235)};
    let recovered = loop (phase = usize(0)) {
        if (phase == usize(0)) {
            continue(usize(1));
        } else {
            let old = loan_write(bank@reserve) { |slot| take_previous(slot, u8(123)) };
            break old;
        }
    };
    let previous_pulse = loan_write(bank@pulse) { |slot| replace(slot, recovered) };
    let final_reserve = bank@reserve;
    let final_pulse = bank@pulse;
    let other_reserve = independent@reserve;
    let other_pulse = independent@pulse;
    return unit;
}
