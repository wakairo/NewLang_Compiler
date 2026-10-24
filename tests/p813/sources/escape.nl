struct Valve { level:u8, marker:u8 }
fn main()->unit {
 let valve=Valve{level:u8(4),marker:u8(210)};
 let outside=loan_write(valve@level){|w|w};outside;unit
}
