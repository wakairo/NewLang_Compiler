struct EscapeGuard { hot:u8, cold:u8 }
fn main()->unit {
 let object=EscapeGuard{cold:u8(7),hot:u8(3)};
 let dangling=loan_write(object@hot){|out|out};
 dangling;unit
}
