struct WindowBounds { floor:u8, ceiling:u8 }
fn exchange(destination:ref<write,u8>,incoming:u8)->u8 {
    let previous=replace(destination,incoming);
    return previous;
}
fn main()->unit {
    let destination=WindowBounds{ceiling:u8(203),floor:u8(6)};
    let archive=WindowBounds{floor:u8(71),ceiling:u8(45)};
    let saved=loan_write(destination@ceiling){|handle|exchange(handle,u8(14))};
    let displaced=loan_write(archive@floor){|handle|replace(handle,saved)};
    let check=destination@ceiling;
    let other=destination@floor;
    let transferred=archive@floor;
    saved;displaced;check;other;transferred;unit
}
