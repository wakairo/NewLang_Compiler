struct ChargeState { label:u8, target:u8 }
fn swap_value(port:ref<write,u8>, fresh:u8)->u8 {
    let old=replace(port,fresh);return old;
}
fn main()->unit {
    let running=ChargeState{target:u8(54),label:u8(181)};
    let audit=ChargeState{label:u8(26),target:u8(220)};
    let seen=usize(4);let stop=usize(9);
    let old=if(seen<stop) {
        let first=loan_write(running@target){|w|swap_value(w,u8(97))};
        let second=loan_write(audit@label){|w|swap_value(w,first)};
        first;second;return unit;
    } else {
        let first=loan_write(running@target){|w|swap_value(w,u8(148))};
        let second=loan_write(audit@label){|w|swap_value(w,first)};
        second;first
    };
    let current=running@target;
    let final_old=loan_write(audit@target){|w|swap_value(w,current)};
    let untouched=running@label;let saved=audit@label;let final=audit@target;
    old;current;final_old;untouched;saved;final;unit
}
