struct Node { next: Option<ptr<Node>>, payload: u8, }
fn main()->unit {
    let anchor=Node{next:Option<ptr<Node>>::None,payload:u8(5)};
    let spare=Node{next:Option<ptr<Node>>::None,payload:u8(9)};
    let observed=anchor@next;
    match observed {
        None=>{let old=loan_write(anchor@next){|w|replace(w,Option<ptr<Node>>::None)};old;unit},
        Some(q)=>{unit},
    };
    unit
}
