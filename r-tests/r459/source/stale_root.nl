struct Node { next: Option<ptr<Node>>, payload: u8, }
fn main() -> unit {
    let dangling = {
        let anchor = Node { next: Option<ptr<Node>>::None, payload: u8(37) };
        loan_read(anchor) { |borrowed| ptr_from_ref(borrowed) }
    };
    loan_read_ptr(dangling) { |late| unit };
    unit
}
