struct SignalCrate { note: u8, tone: u8 }
fn displace_tone(destination: ref<write, u8>, incoming: u8) -> u8 {
    return replace(destination, incoming);
}
fn main() -> unit {
    let ambient = SignalCrate{note: u8(68), tone: u8(152)};
    let selected = SignalCrate{note: u8(47), tone: u8(201)};
    let recovered = loop (phase = usize(3)) {
        if (phase < usize(4)) {
            let prior = loan_write(selected@tone) { |write| displace_tone(write, u8(137)) };
            break prior;
        } else {
            continue(phase - usize(4));
        }
    };
    let displaced = loan_write(selected@note) { |write| replace(write, recovered) };
    let note = selected@note;
    let tone = selected@tone;
    let other_note = ambient@note;
    let other_tone = ambient@tone;
    return unit;
}
