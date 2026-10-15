struct Node { next: Option<ptr<Node>>, payload: u8, }
fn main() -> unit {
    let anchor = Node { next: Option<ptr<Node>>::None, payload: u8(37) };
    let escaped = loan_read(anchor) { |borrowed| borrowed };
    escaped;
    unit
}
