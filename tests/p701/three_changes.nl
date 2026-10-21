struct CounterState { first:u8, second:u8 }
fn next_value()->u8 {
    let state=CounterState{first:u8(12),second:u8(38)};
    let a=loan_write(state@first){|out|replace(out,u8(61))};
    let b=loan_write(state@second){|out|replace(out,a)};
    let c=loan_write(state@first){|out|replace(out,b)};
    c;
    state@first
}
fn main()->unit {
    let value=next_value();
    let target=CounterState{first:u8(3),second:u8(4)};
    let old=loan_write(target@first){|out|replace(out,value)};
    let seen=target@first;
    let sibling=target@second;
    old;seen;sibling;unit
}
