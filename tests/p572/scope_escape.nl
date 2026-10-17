struct Node { next: Option<ptr<Node>>, payload: u8, }
fn main()->unit {
    let middle=Node{next:Option<ptr<Node>>::None,payload:u8(22)};
    let p_middle=loan_read(middle){|m|ptr_from_ref(m)};
    let left=Node{next:Option<ptr<Node>>::Some(p_middle),payload:u8(11)};
    let p_left=loan_read(left){|l|ptr_from_ref(l)};
    let right=Node{next:Option<ptr<Node>>::None,payload:u8(33)};
    let p_right=loan_read(right){|r|ptr_from_ref(r)};
    let escaped=loan_write(left@next){|wl|wl};
    let old_right=loan_write(right@next){|wr|
        replace(wr,Option<ptr<Node>>::Some(p_left))
    };
    let final_left=left@next;
    let final_right=right@next;
    let untouched_middle=middle@next;
    unit
}
