struct Pair { left:u8, right:u8, }
fn main()->unit {
    let spent=usize(17); let capacity=usize(23);
    let original=Pair{left:u8(27),right:u8(201)};
    let within=spent < capacity;
    let choice=if(within){u8(91)}else{u8(13)};
    let former=loan_write(original@left){|w|replace(w,choice)};
    let after=original@left; let sibling=original@right;
    former; after; sibling; unit
}
