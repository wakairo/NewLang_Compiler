struct PumpLimits { setting:u8, tag:u8 }
fn exchange(target:ref<write,u8>, incoming:u8)->u8 {
    let displaced=replace(target,incoming);return displaced;
}
fn main()->unit {
    let active=PumpLimits{setting:u8(43),tag:u8(198)};
    let archive=PumpLimits{setting:u8(17),tag:u8(211)};
    let count=usize(2);let boundary=usize(6);
    let previous=if(count<boundary) {
        let stopped=loan_write(active@setting){|writer|exchange(writer,u8(73))};
        stopped;return unit;
    } else {
        loan_write(active@setting){|writer|exchange(writer,u8(126))}
    };
    let current=active@setting;
    let archived=loan_write(archive@tag){|writer|exchange(writer,current)};
    let sibling=active@tag;let saved=archive@tag;let stable=archive@setting;
    previous;current;archived;sibling;saved;stable;unit
}
