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
        Some(q)=>{let old_tail=loan_write(tail@next){|w|replace(w,Option<ptr<Node>>::Some(q))};old_tail;unit},
    };
    let joined_tail=tail@next;
    joined_tail;
    match joined_tail {
        None=>{unit},
        Some(p)=>{loan_read_ptr(p){|tail_ref|unit}},
    };
    let old_some=loan_write(head@next){|w|
        replace(w,Option<ptr<Node>>::None)
    };
    unit
}
