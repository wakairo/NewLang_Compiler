struct Node { next: Option<ptr<Node>>, payload: u8, }

fn main()->unit {
    let peer = Node { next: Option<ptr<Node>>::None, payload: u8(83) };
    let peer_ptr = loan_read(peer) { |r| ptr_from_ref(r) };
    let anchor = Node { next: Option<ptr<Node>>::None, payload: u8(47) };
    let pre_old = loan_write(anchor@next) { |w|
        replace(w, Option<ptr<Node>>::Some(peer_ptr))
    };
    pre_old;
    let selector = anchor@next;
    match selector {
        None => {
            unit
        },
        Some(found) => {
            let previous = loan_write(anchor@next) { |w|
                replace(w, Option<ptr<Node>>::None)
            };
            previous;
            found;
            unit
        },
    };
    let final_link = anchor@next;
    final_link;
    unit
}
