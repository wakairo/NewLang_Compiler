struct AlarmBand { threshold:u8, marker:u8 }
fn main()->unit {
 let alarm=AlarmBand{marker:u8(193),threshold:u8(8)};
 let measured=usize(2);let limit=usize(6);
 let prior=if (measured < limit) {
   loan_write(alarm@threshold){|slot|replace(slot,u8(64))}
 } else {
   alarm@threshold
 };
 let present=alarm@threshold;let retained=alarm@marker;
 prior;present;retained;unit
}
