struct SignalState { tag:u8, count:u8 }
fn main()->unit {
    let supplied=rotate_signal();
    let destination=SignalState{count:u8(3),tag:u8(4)};
    let previous=loan_write(destination@count){|out|replace(out,supplied)};
    let current=destination@count;
    let sibling=destination@tag;
    previous;current;sibling;unit
}
fn rotate_signal()->u8 {
    let state=SignalState{count:u8(207),tag:u8(9)};
    let old_count=loan_write(state@count){|out|replace(out,u8(31))};
    let old_tag=loan_write(state@tag){|out|replace(out,old_count)};
    old_tag;
    return state@tag;
}
