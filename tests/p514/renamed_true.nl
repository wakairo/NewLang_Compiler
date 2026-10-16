struct Pair { left:u8, right:u8, }
fn main()->unit {
    let original=Pair{left:u8(0),right:u8(9)};
    let a=usize(2);
    let b=usize(2);
    let equal=a==b;
    let seven=u8(7);let eight=u8(8);let decision=equal;
    let selected=if (decision) { seven } else { eight };
    let copied=selected;
    let old=loan_write(original@left){|w|replace(w,copied)};
    let after=original@left;let sibling=original@right;
    old;after;sibling;unit
}
