fn wire_size(payload: usize) -> usize {
    if (payload < usize(497)) {
        let frame = payload + usize(16);
        frame
    } else {
        payload
    }
}

fn main() -> usize {
    let size = wire_size(usize(496));
    let saved = size;
    saved
}
