struct Node { next: Option<ptr<Node>>, payload: u8, }
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
        Some(q)=>{loan_read_ptr(q){|tail_ref|unit}},
    };
    let old_some=loan_write(head@next){|w|
        replace(w,Option<ptr<Node>>::None)
    };
    unit
}
