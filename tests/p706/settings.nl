struct Settings { port:u8, mode:u8 }
fn update(field:ref<write,u8>,next:u8)->u8{replace(field,next)}
fn main()->unit {
    let original=Settings{port:u8(31),mode:u8(94)};
    let receiver=Settings{mode:u8(52),port:u8(18)};
    let old=loan_write(original@port){|slot|update(slot,u8(167))};
    let before_receiver=loan_write(receiver@mode){|out|replace(out,old)};
    let current=original@port;
    let sibling=original@mode;
    let received=receiver@mode;
    before_receiver;current;sibling;received;unit
}
