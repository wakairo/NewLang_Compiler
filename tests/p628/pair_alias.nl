struct Pair { left:u8, right:u8, }
fn main()->unit {
    let spent=usize(19); let capacity=usize(31);
    let original=Pair{left:u8(27),right:u8(201)};
    let within=spent < capacity;
    let condition_copy=within;
    let choice=if(condition_copy){u8(91)}else{u8(13)};
    let former=loan_write(original@left){|w|replace(w,choice)};
    let after=original@left; let sibling=original@right;
    former; after; sibling; unit
}
