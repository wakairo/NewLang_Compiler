struct SensorState { marker:u8, level:u8 }
fn main()->unit {
 let meter=SensorState{level:u8(57),marker:u8(206)};
 let sample=usize(2);let limit=usize(5);
 let previous=if (sample < limit) {
   return u8(6);
 } else {
   loan_write(meter@level){|access|replace(access,u8(91))}
 };
 let now=meter@level;let peer=meter@marker;
 previous;now;peer;unit
}
