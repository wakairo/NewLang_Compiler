struct Pair { left:u8, right:u8, }
fn main()->unit {
    let a=usize(2);
    let b=usize(2);
    let equal=a==b;
    let p=Pair{left:u8(0),right:u8(9)};
    if (equal) { let old=loan_write(p@left){|w|replace(w,u8(7))};old;unit }
    else { let old=loan_write(p@left){|w|replace(w,u8(8))};old;unit };
    let after=p@left;after;let sibling=p@right;sibling;unit
}
