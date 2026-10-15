struct Node { next: Option<ptr<Node>>, payload: u8, }
fn main()->unit {
    let seed=Node{next:Option<ptr<Node>>::None,payload:u8(29)};
    let peer_ptr=loan_read(seed){|stable|ptr_from_ref(stable)};
    let item=Node{next:Option<ptr<Node>>::Some(peer_ptr),payload:u8(61)};
    let observed=item@next;
    observed;
    unit
}
