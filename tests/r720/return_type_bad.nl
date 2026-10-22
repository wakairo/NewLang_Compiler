struct TypeFence { primary:u8, peer:u8 }
fn main()->unit {
 let root=TypeFence{peer:u8(42),primary:u8(17)};
 let step=usize(3);let end=usize(8);
 let former=if (step < end) {
   return u8(91);
 } else {
   loan_write(root@primary){|writer|replace(writer,u8(99))}
 };
 let current=root@primary;
 former;current;unit
}
