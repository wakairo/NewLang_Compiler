struct Pair { left:u8, right:u8, }
fn main()->unit {
    let demand=usize(84); let limit=usize(20);
    let record=Pair{left:u8(11),right:u8(243)};
    let permitted=demand < limit;
    let chosen=if(permitted){let step=if(permitted){u8(142)}else{u8(39)};step}else{let step=u8(37);step};
    let prior=loan_write(record@left){|cell|replace(cell,chosen)};
    let current=record@left; let retained=record@right;
    prior; current; retained; unit
}
