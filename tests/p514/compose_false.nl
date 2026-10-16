struct Pair { left:u8, right:u8, }
fn main()->unit {
    let p=Pair{left:u8(0),right:u8(9)};
    let a=usize(2);
    let b=usize(3);
    let equal=a==b;
    let selected=if (equal) { u8(7) } else { u8(8) };
    let old=loan_write(p@left){|w|replace(w,selected)};
    let after=p@left;let sibling=p@right;
    old;after;sibling;unit
}
