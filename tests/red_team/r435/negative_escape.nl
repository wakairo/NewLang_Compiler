struct Node { next: Option<ptr<Node>>, payload: u8, }
fn main() -> unit {
    let anchor = Node { next: Option<ptr<Node>>::None, payload: u8(31) };
    let escaped = loan_write(anchor@next) { |w|
        w
    };
    let illegal = replace(escaped, Option<ptr<Node>>::None);
    unit
}
