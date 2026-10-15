struct Node { next: Option<ptr<Node>>, payload: u8, }
fn main() -> unit {
    let anchor = Node { next: Option<ptr<Node>>::None, payload: u8(37) };
    let peer = Node { next: Option<ptr<Node>>::None, payload: u8(211) };
    let snapshot = peer@next;
    match snapshot {
        None => { unit },
        Some(q) => { unit },
    };
    unit
}
