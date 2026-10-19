struct Pair { left:u8, right:u8, }
fn main()->unit {
    let demand=u8(21); let limit=usize(45);
    let record=Pair{left:u8(18),right:u8(209)};
    let permitted=demand < limit;
    let chosen=if(permitted){let staged=u8(83);staged}else{u8(24)};
    let prior=loan_write(record@left){|cell|replace(cell,chosen)};
    let current=record@left; let retained=record@right;
    prior; current; retained; unit
}
