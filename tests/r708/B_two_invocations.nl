struct Orbit { top:u8, bottom:u8 }
fn shift(target:ref<write,u8>, next:u8)->u8 {
    replace(target,next)
}
fn main()->unit {
    let lead=Orbit{bottom:u8(16),top:u8(77)};
    let follow=Orbit{top:u8(21),bottom:u8(66)};
    let old_lead=loan_write(lead@top){|slot|
        shift(slot,u8(98))
    };
    let old_follow=loan_write(follow@bottom){|slot|
        shift(slot,old_lead)
    };
    let lead_top=lead@top;
    let lead_bottom=lead@bottom;
    let follow_top=follow@top;
    let follow_bottom=follow@bottom;
    old_lead;old_follow;lead_top;lead_bottom;follow_top;follow_bottom;unit
}
