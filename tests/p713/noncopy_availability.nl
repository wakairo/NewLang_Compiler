struct ValveProfile { coast:u8, drive:u8 }
fn main()->unit {
 let settings=ValveProfile{drive:u8(111),coast:u8(42)};
 let lifetime=lifetime_domain();
 let demand=usize(3);let ceiling=usize(9);
 let before=if (demand < ceiling) {
   finalize_domain(lifetime);
   loan_write(settings@drive){|writer|replace(writer,u8(205))}
 } else {
   loan_write(settings@drive){|writer|replace(writer,u8(17))}
 };
 let current=settings@drive;let sibling=settings@coast;
 before;current;sibling;finalize_domain(lifetime);unit
}
