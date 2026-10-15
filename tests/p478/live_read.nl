struct Node { next: Option<ptr<Node>>, payload: u8, }
fn set_link(field:ref<write,Option<ptr<Node>>>,next:Option<ptr<Node>>)->Option<ptr<Node>> {
    replace(field,next)
}
fn main()->unit {
    let tail=Node{next:Option<ptr<Node>>::None,payload:u8(2)};
    let tail_ptr=loan_read(tail){|r|ptr_from_ref(r)};
    let head=Node{next:Option<ptr<Node>>::None,payload:u8(1)};
    let old_none=loan_write(head@next){|w|
        replace(w,Option<ptr<Node>>::Some(tail_ptr))
    };
    let observed=head@next;
    match observed {
        None=>{unit},
        Some(q)=>{let old_tail=loan_read(tail){|held|let old=loan_write(tail@next){|w|set_link(w,Option<ptr<Node>>::Some(q))};held;old};old_tail;unit},
    };
    let old_some=loan_write(head@next){|w|
        replace(w,Option<ptr<Node>>::None)
    };
    unit
}
