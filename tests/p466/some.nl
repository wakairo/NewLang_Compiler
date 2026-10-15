struct Node { next: Option<ptr<Node>>, payload: u8, }
fn main()->unit {
    let anchor=Node{next:Option<ptr<Node>>::None,payload:u8(29)};
    let anchor_ptr=loan_read(anchor){|stable|ptr_from_ref(stable)};
    let peer=Node{next:Option<ptr<Node>>::Some(anchor_ptr),payload:u8(61)};
    let observed=peer@next;
    match observed {
        None=>{let old=loan_write(peer@next){|w|replace(w,Option<ptr<Node>>::Some(anchor_ptr))};old;unit},
        Some(q)=>{let old=loan_write(peer@next){|w|replace(w,Option<ptr<Node>>::None)};old;unit},
    };
    let joined=peer@next;
    joined;
    loan_read_ptr(anchor_ptr){|original_anchor|unit};
    unit
}
