struct FanProgram { serial:u8, demand:u8 }
fn write_on_equal(output:ref<write,u8>, supplied:u8)->unit {
    let queued=usize(81);let configured=usize(81);
    if(queued == configured) {
        let former=replace(output,supplied);former;unit
    } else {
        return unit;
    };unit
}
fn main()->unit {
    let running=FanProgram{demand:u8(57),serial:u8(167)};
    let trace=FanProgram{serial:u8(31),demand:u8(208)};
    let initial=running@demand;
    loan_write(running@demand){|channel|write_on_equal(channel,u8(132))};
    let now=running@demand;let untouched=running@serial;
    let previous=loan_write(trace@serial){|channel|replace(channel,now)};
    let recorded=trace@serial;
    initial;now;untouched;previous;recorded;unit
}
