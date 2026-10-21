struct Ledger { guard:u8, balance:u8 }
fn old_balance()->u8 {
    let inner = Ledger { balance:u8(37), guard:u8(91) };
    let before = loan_write(inner@balance) { |writer| replace(writer,u8(182)) };
    let after = inner@balance;
    let protected = inner@guard;
    after; protected;
    return before;
}
fn main()->unit {
    let outer = Ledger { balance:u8(6), guard:u8(43) };
    let received = old_balance();
    let displaced = loan_write(outer@balance) { |writer| replace(writer,received) };
    let observed = outer@balance;
    let unaffected = outer@guard;
    displaced; observed; unaffected; unit
}
