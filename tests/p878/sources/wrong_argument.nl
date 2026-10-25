struct RelayConfiguration { level:u8, tag:u8 }
fn apply_setting(write_ref:ref<write,u8>,new_setting:u8)->unit {
    if (usize(18) == usize(18)) {
        let displaced = replace(write_ref,new_setting);
        unit
    } else {
        return unit;
    }
}
fn main()->unit {
    let settings = RelayConfiguration{level:u8(82),tag:u8(199)};
    let sink = RelayConfiguration{level:u8(23),tag:u8(141)};
    let before = settings@level;
    let completed = loan_write(settings@level){|active|apply_setting(active,usize(156))};
    let current = settings@level;
    let sibling = settings@tag;
    let sink_before = loan_write(sink@tag){|active|replace(active,current)};
    let sink_current = sink@tag;
    unit
}
