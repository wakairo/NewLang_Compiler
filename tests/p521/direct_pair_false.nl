struct Pair { left:u8, right:u8, }
fn main()->unit {
    let a=usize(29); let b=usize(30);
    let original=Pair{left:u8(37),right:u8(211)};
    let selected=if(a==b){u8(108)}else{u8(173)};
    let former=loan_write(original@left){|w|replace(w,selected)};
    let after=original@left; let sibling=original@right;
    former; after; sibling; unit
}
