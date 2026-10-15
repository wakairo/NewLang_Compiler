struct Node { next: Option<ptr<Node>>, payload: u8, }

fn main()->unit {
    let peer = Node { next: Option<ptr<Node>>::None, payload: u8(79) };
    let peer_ptr = loan_read(peer) { |r| ptr_from_ref(r) };
    let anchor = Node { next: Option<ptr<Node>>::None, payload: u8(31) };
    let selector = anchor@next;
    match selector {
        None => {
            let previous = loan_write(anchor@next) { |w|
                replace(w, Option<ptr<Node>>::Some(peer_ptr))
            };
            previous;
            unit
        },
        Some(found) => {
            found;
            unit
        },
    };
    let final_link = anchor@next;
    final_link;
    unit
}
