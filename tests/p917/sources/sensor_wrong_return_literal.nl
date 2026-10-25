struct SensorLatch { sample:u8, marker:u8 }
fn exchange(slot:ref<write,u8>,proposed:u8)->u8 {
    let original=replace(slot,proposed);
    let saved=original;
    return u8(231);
}
fn main()->unit {
    let live=SensorLatch{sample:u8(231),marker:u8(6)};
    let frame=SensorLatch{sample:u8(48),marker:u8(155)};
    if (usize(3) < usize(8)) {
        loan_write(live@sample){|r|exchange(r,u8(19))}
    } else {
        loan_write(live@sample){|r|exchange(r,u8(173))}
    };
    let current=live@sample;
    current;unit
}
