struct OrbitTile { ember: u8, reserve: u8 }
fn main() -> unit {
    let capsule = OrbitTile { reserve: u8(88), ember: u8(21) };
    let before = capsule@reserve;
    let settled = loop (grain = usize(0)) {
        if (grain == usize(0)) {
            break grain;
        } else {
            continue(grain - usize(1));
        }
    };
    let echoed = settled;
    let displaced = if (echoed == usize(1)) {
        loan_write(capsule@reserve) { |permit| replace(permit, u8(41)) }
    } else {
        loan_write(capsule@reserve) { |permit| replace(permit, u8(67)) }
    };
    let after = capsule@reserve;
    let still = capsule@ember;
    displaced;
    unit
}
