struct Pair { left:u8, right:u8, }
fn main()->unit {
    let volume=usize(9); let limit=usize(34);
    let larch=Pair{left:u8(57),right:u8(209)};
    let willow=Pair{left:u8(131),right:u8(244)};
    let eligible=volume < limit;
    let selected=if(eligible){let warm=u8(186);warm}else{u8(42)};
    let old_larch=loan_write(larch@left){|slot|replace(slot,selected)};
    let reserved=usize(13); let threshold=usize(13);
    let can_forward=reserved < threshold;
    let carried=old_larch;
    let old_willow=loan_write(willow@left){|slot|replace(slot,carried)};
    let larch_left=larch@left; let larch_right=larch@right;
    let willow_left=willow@left; let willow_right=willow@right;
    old_larch; old_willow;
    larch_left; larch_right; willow_left; willow_right; unit
}
