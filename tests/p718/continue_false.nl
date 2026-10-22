struct QueueTuning { quota:u8, stamp:u8 }
fn main()->unit {
 let cap=usize(7);let waiting=usize(11);
 let tuning=QueueTuning{stamp:u8(144),quota:u8(23)};
 let earlier=if (waiting < cap) {
   loan_write(tuning@quota){|writer|replace(writer,u8(38))}
 } else {
   let discarded=loan_write(tuning@quota){|ending|replace(ending,u8(219))};
   discarded;return unit;
 };
 let current=tuning@quota;let sibling=tuning@stamp;
 earlier;current;sibling;unit
}
