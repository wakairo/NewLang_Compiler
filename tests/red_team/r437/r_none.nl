struct Node { next: Option<ptr<Node>>, payload: u8, }

fn main()->unit {
    let peer = Node { next: Option<ptr<Node>>::None, payload: u8(79) };
    let peer_ptr = loan_read(peer) { |peer_ref| ptr_from_ref(peer_ref) };
    let anchor = Node { next: Option<ptr<Node>>::None, payload: u8(31) };
    let flag = Option<ptr<Node>>::None;
    match flag {
        None => {
            let previous = loan_write(anchor@next) { |out|
                replace(out, Option<ptr<Node>>::Some(peer_ptr))
            };
            previous;
            unit
        },
        Some(found) => {
            found;
            unit
        },
    };
    let after = anchor@next;
    after;
    unit
}
