struct FanTuning { idle:u8, peak:u8 }
fn main()->unit{let got=peak_after_write();let dst=FanTuning{idle:u8(3),peak:u8(4)};let old=loan_write(dst@idle){|w|replace(w,got)};let seen=dst@idle;old;seen;unit}
fn peak_after_write()->u8 {
    let fan=FanTuning{peak:u8(64),idle:u8(11)};
    let previous=loan_write(fan@peak){|slot|slot};
    previous;
    fan@peak
}
