struct Pair { left:u8, right:u8, }
fn main()->unit {
    let demand=usize(8); let limit=usize(90);
    let record=Pair{left:u8(55),right:u8(202)};
    let permitted=demand < limit;
    let chosen=if(permitted){let shadow=u8(67);shadow}else{let shadow=u8(67);shadow};
    let prior=loan_write(record@left){|cell|replace(cell,chosen)};
    let current=record@left; let retained=record@right;
    prior; current; retained; unit
}
