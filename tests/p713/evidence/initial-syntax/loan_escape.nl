struct ValveProfile { coast:u8, drive:u8 }
fn main()->unit {
 let settings=ValveProfile{drive:u8(111),coast:u8(42)};
 let demand=usize(3);let ceiling=usize(9);
 let before=if demand < ceiling {
   loan_write(settings@drive){|writer|writer}
 } else {
   loan_write(settings@drive){|writer|replace(writer,u8(17))}
 };
 let current=settings@drive;let sibling=settings@coast;
 before;current;sibling;unit
}
