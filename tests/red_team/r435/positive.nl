struct Node { next: Option<ptr<Node>>, payload: u8, }
fn main() -> unit {
    let anchor = Node { next: Option<ptr<Node>>::None, payload: u8(31) };
    let neighbor = Node { next: Option<ptr<Node>>::None, payload: u8(47) };
    let selector = neighbor@next;
    match selector {
        None => {
            let displaced = loan_write(anchor@next) { |w|
                replace(w, Option<ptr<Node>>::None)
            };
            unit
        },
        Some(selected) => {
            let observed = neighbor@next;
            unit
        },
    };
    let final_anchor = anchor@next;
    let final_neighbor = neighbor@next;
    unit
}
