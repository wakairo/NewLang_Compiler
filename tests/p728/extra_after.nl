struct BrakePanel { status:u8, pressure:u8 }
fn vent(port:ref<write,u8>,amount:u8)->unit {
    let measured=usize(12);let threshold=usize(9);
    if(measured < threshold){return unit;}else{let previous=replace(port,amount);previous;unit};unit
}
fn main()->unit {
    let device=BrakePanel{pressure:u8(53),status:u8(174)};
    let ledger=BrakePanel{status:u8(26),pressure:u8(91)};
    let before=device@pressure;let identifier=device@status;
    loan_write(device@pressure){|channel|vent(channel,u8(106));u8(4);};
    let current=device@pressure;let unchanged=device@status;
    let ledger_old=loan_write(ledger@status){|sink|replace(sink,current)};
    let recorded=ledger@status;before;identifier;current;unchanged;ledger_old;recorded;unit
}
