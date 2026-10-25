struct CounterTune { idle:u8, step:u8 }
fn overwrite(cell:ref<write,u8>,proposed:u8)->u8 {
    let prior=replace(cell,proposed);
    return prior;
}
fn main()->unit {
    let counter=CounterTune{idle:u8(36),step:u8(94)};
    let spare=CounterTune{step:u8(61),idle:u8(182)};
    if (usize(12) < usize(7)) {
        loan_write(counter@step){|w|overwrite(w,u8(13))}
    } else {
        loan_write(counter@step){|w|overwrite(w,u8(207))}
    };
    let current=counter@step;
    current;unit
}
