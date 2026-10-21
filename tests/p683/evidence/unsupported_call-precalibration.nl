fn echo(x:u8)->u8{x}
struct DeviceSettings { port: u8, mode: u8 }
fn main() -> u8 {
    let device = DeviceSettings { mode: u8(31), port: u8(7) };
    let saved_port = device@port;
    let old_mode = loan_write(device@mode) { |w| replace(w, saved_port) };
    let old_port = loan_write(device@port) { |w| replace(w, old_mode) };
    let port_after = device@port;
    let mode_after = device@mode;
    saved_port;
    old_mode;
    old_port;
    port_after;
    mode_after;
    echo(port_after)
}
