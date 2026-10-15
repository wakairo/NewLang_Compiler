struct Node { next: Option<ptr<Node>>, payload: u8, }

fn main()->unit {
    let peer = Node { next: Option<ptr<Node>>::None, payload: u8(83) };
    let peer_ptr = loan_read(peer) { |peer_ref| ptr_from_ref(peer_ref) };
    let anchor = Node { next: Option<ptr<Node>>::Some(peer_ptr), payload: u8(47) };
    let flag = Option<ptr<Node>>::Some(peer_ptr);
    match flag {
        None => {
            unit
        },
        Some(found) => {
            let previous = loan_write(anchor@next) { |out|
                replace(out, Option<ptr<Node>>::None)
            };
            previous;
            found;
            unit
        },
    };
    let after = anchor@next;
    after;
    unit
}
