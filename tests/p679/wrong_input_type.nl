struct DeviceSettings { port: u8, mode: u8 }
fn main() -> unit {
    let device = DeviceSettings { mode: u8(31), port: u8(7) };
    let saved_port = device@port;
    let old_mode = loan_write(device@mode) { |w| replace(w, saved_port) };
    let old_port = loan_write(device@port) { |w| replace(w, usize(31)) };
    let port_after = device@port;
    let mode_after = device@mode;
    saved_port;
    old_mode;
    old_port;
    port_after;
    mode_after;
    unit
}
