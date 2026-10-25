struct TransferBudget { available:u8, marker:u8 }
fn exchange_budget(slot:ref<write,u8>,fresh:u8)->u8 {
    let old=replace(slot,fresh);
    return old;
}
fn main()->unit {
    let active=TransferBudget{marker:u8(203),available:u8(42)};
    let archive=TransferBudget{available:u8(17),marker:u8(91)};
    let chosen=if (usize(3) < usize(6)) {
        loan_write(active@available){|w|exchange_budget(w,u8(76))}
    } else {
        loan_write(active@available){|w|exchange_budget(w,u8(154))}
    };
    let current=active@available;
    let displaced=loan_write(archive@marker){|w|exchange_budget(w,current)};
    let retained=active@marker;let saved=archive@marker;
    chosen;current;displaced;retained;saved;unit
}
