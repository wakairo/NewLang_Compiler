struct Node { next: Option<ptr<Node>>, payload: u8, }
fn main() -> unit {
    let anchor = Node { next: Option<ptr<Node>>::None, payload: u8(37) };
    let pin = loan_read(anchor) { |borrowed| ptr_from_ref(borrowed) };
    let peer = Node { next: Option<ptr<Node>>::Some(pin), payload: u8(211) };
    let snapshot = peer@next;
    match snapshot {
        None => { unit },
        Some(q) => { loan_read_ptr(q) { |observed| unit } },
    };
    let anchor_link = anchor@next;
    match anchor_link {
        None => { unit },
        Some(q) => { unit },
    };
    unit
}
