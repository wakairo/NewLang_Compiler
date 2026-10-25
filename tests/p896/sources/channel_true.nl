struct ChannelProfile { label:u8, route:u8 }
fn route_exchange(write_port:ref<write,u8>,incoming:u8)->u8 {
    let previous=replace(write_port,incoming);
    let retained=previous;
    return retained;
}
fn main()->unit {
    let active=ChannelProfile{route:u8(73),label:u8(19)};
    let archive=ChannelProfile{label:u8(161),route:u8(10)};
    let old=if (usize(3) < usize(8)) {
        loan_write(active@route){|w|route_exchange(w,u8(103))}
    } else {
        loan_write(active@route){|w|route_exchange(w,u8(217))}
    };
    let current=active@route;
    let removed=loan_write(archive@label){|w|route_exchange(w,current)};
    old;current;removed;unit
}
