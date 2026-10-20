struct Pair { left:u8, right:u8, }
fn main()->unit {
    let volume=usize(43); let limit=usize(11);
    let sable=Pair{left:u8(28),right:u8(183)};
    let birch=Pair{left:u8(87),right:u8(164)};
    let eligible=volume < limit;
    let selected=if(eligible){let warm=u8(152);warm}else{u8(19)};
    let old_sable=loan_write(sable@left){|slot|replace(slot,selected)};
    let reserved=usize(23); let threshold=usize(23);
    let can_forward=reserved < threshold;
    let carried=old_sable;
    let old_birch=loan_write(birch@left){|slot|replace(slot,carried)};
    let sable_left=sable@left; let sable_right=sable@right;
    let birch_left=birch@left; let birch_right=birch@right;
    old_sable; old_birch;
    sable_left; sable_right; birch_left; birch_right; unit
}
