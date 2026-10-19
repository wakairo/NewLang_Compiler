struct Pair { left:u8, right:u8, }
fn main()->unit {
    let demand=usize(21); let limit=usize(45);
    let record=Pair{left:u8(18),right:u8(209)};
    let twin=u8(83);let fallback=u8(83);
    let permitted=demand < limit;
    let chosen=if(permitted){let staged=fallback;staged}else{u8(24)};
    let prior=loan_write(record@left){|cell|replace(cell,chosen)};
    let current=record@left; let retained=record@right;
    prior; current; retained; unit
}
