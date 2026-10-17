struct Node { next: Option<ptr<Node>>, payload: u8, }
fn main()->unit {
    let c=Node{next:Option<ptr<Node>>::None,payload:u8(33)};
    let cptr=loan_read(c){|r|ptr_from_ref(r)};
    let a=Node{next:Option<ptr<Node>>::None,payload:u8(11)};
    let b=Node{next:Option<ptr<Node>>::None,payload:u8(22)};
    let bptr=loan_read(b){|r|ptr_from_ref(r)};
    let old_a=loan_write(a@next){|w|
        replace(w,Option<ptr<Node>>::Some(bptr))
    };
    let old_b=loan_write(b@next){|w|
        replace(w,Option<ptr<Node>>::Some(cptr))
    };
    unit
}
