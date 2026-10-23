struct RouterTune { retry:u8, ceiling:u8 }
fn exchange_setting(target:ref<write,u8>, incoming:u8)->u8 {
    let previous=replace(target,incoming);
    return previous;
}
fn main()->unit {
    let live=RouterTune{retry:u8(12),ceiling:u8(67)};
    let history=RouterTune{retry:u8(29),ceiling:u8(185)};
    let waiting=usize(2);let limit=usize(8);
    let earlier=if (waiting < limit) {
        loan_write(live@absent){|writer|exchange_setting(writer,u8(94))}
    } else {
        loan_write(live@ceiling){|writer|exchange_setting(writer,u8(133))}
    };
    let actual_current=live@ceiling;
    let displaced=loan_write(history@retry){|writer|exchange_setting(writer,actual_current)};
    let now=live@ceiling;let sibling=live@retry;
    let recorded=history@retry;let preserved=history@ceiling;
    earlier;actual_current;displaced;now;sibling;recorded;preserved;unit
}
