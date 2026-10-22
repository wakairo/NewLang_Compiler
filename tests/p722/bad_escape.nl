struct RelaySettings { gain:u8, tag:u8 }
fn adjust(field:ref<write,u8>,incoming:u8)->unit {
    let sample=usize(2);let limit=usize(5);
    if (sample < limit) { return unit; } else {
        let displaced=replace(field,incoming);displaced;unit
    };
    unit
}
fn main()->unit {
    let sensor=RelaySettings{tag:u8(193),gain:u8(41)};
    let archive=RelaySettings{gain:u8(16),tag:u8(67)};
    loan_write(sensor@gain){|handle|handle};
    let current=sensor@gain;let sibling=sensor@tag;
    let prior=loan_write(archive@tag){|out|replace(out,current)};
    let received=archive@tag;prior;current;sibling;received;unit
}
