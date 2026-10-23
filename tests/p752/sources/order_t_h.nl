fn main()->unit{
    let current=Controls{ceiling:u8(144),trim:u8(28)};
    let archive=Controls{trim:u8(2),ceiling:u8(177)};
    let old=loan_write(current@trim){|w|retune(w,requested())};
    let displaced=loan_write(archive@ceiling){|w|replace(w,old)};
    old;displaced;unit
}
struct Controls { trim:u8, ceiling:u8 }
fn retune(field:ref<write,u8>,next:u8)->u8{let old=replace(field,next);return old;}
struct Packet { chain:Option<ptr<Packet>>, token:u8 }
fn requested()->u8{u8(93)}
