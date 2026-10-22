struct WindowState { seal:u8, ceiling:u8 }
fn revise(destination:ref<write,u8>,next:u8)->unit {
    let queued=usize(3);let capacity=usize(7);
    if (queued < capacity) {
        let saved=replace(destination,next);saved;unit
    } else {
        let saved=replace(destination,u8(219));saved;return unit;
    };
    unit
}
fn main()->unit {
    let window=WindowState{ceiling:u8(29),seal:u8(151)};
    let journal=WindowState{seal:u8(72),ceiling:u8(9)};
    loan_write(window@ceiling){|writer|revise(writer,u8(62))};
    let current=window@ceiling;let sibling=window@seal;
    let displaced=loan_write(journal@seal){|sink|replace(sink,current)};
    let recorded=journal@seal;current;sibling;displaced;recorded;unit
}
