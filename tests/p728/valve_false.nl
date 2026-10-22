struct ValvePanel { opening:u8, serial:u8 }
fn route(port:ref<write,u8>,amount:u8)->unit {
    let measured=usize(13);let threshold=usize(8);
    if(measured < threshold){let previous=replace(port,u8(224));previous;return unit;}else{let previous=replace(port,amount);previous;unit};unit
}
fn main()->unit {
    let device=ValvePanel{serial:u8(207),opening:u8(38)};
    let ledger=ValvePanel{opening:u8(68),serial:u8(14)};
    let before=device@opening;let identifier=device@serial;
    loan_write(device@opening){|channel|route(channel,u8(82));};
    let current=device@opening;let unchanged=device@serial;
    let ledger_old=loan_write(ledger@serial){|sink|replace(sink,current)};
    let recorded=ledger@serial;before;identifier;current;unchanged;ledger_old;recorded;unit
}
