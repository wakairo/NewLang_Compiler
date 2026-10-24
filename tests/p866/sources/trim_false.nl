struct TrimSetting { command:u8, label:u8 }
fn write_unless_equal(target:ref<write,u8>, fresh:u8)->unit {
    if(usize(46) == usize(97)) {
        return unit;
    } else {
        let previous=replace(target,fresh);previous;unit
    };unit
}
fn main()->unit {
    let active=TrimSetting{label:u8(183),command:u8(37)};
    let mirror=TrimSetting{command:u8(68),label:u8(214)};
    let before=active@command;
    loan_write(active@command){|port|write_unless_equal(port,u8(109))};
    let current=active@command;let other=active@label;
    let displaced=loan_write(mirror@label){|port|replace(port,current)};
    let saved=mirror@label;
    before;current;other;displaced;saved;unit
}
