struct MeterState { alarm:u8, scale:u8 }
fn recalibrated_scale()->u8 {
    let meter=MeterState{alarm:u8(137),scale:u8(29)};
    let old_alarm=loan_write(meter@alarm){|a|replace(a,u8(201))};
    let old_scale=loan_write(meter@scale){|b|replace(b,old_alarm)};
    old_scale;
    let current=meter@scale;
    return current;
}
fn main()->unit{let got=recalibrated_scale();let dst=MeterState{alarm:u8(3),scale:u8(4)};let old=loan_write(dst@alarm){|w|replace(w,got)};let seen=dst@alarm;old;seen;unit}
