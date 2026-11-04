struct OrbitTile { ember: u8, reserve: u8 }
fn main() -> unit {
    let capsule = OrbitTile { reserve: u8(57), ember: u8(33) };
    let before = capsule@ember;
    let settled = loop (grain = usize(3)) {
        if (grain < usize(7)) {
            continue(grain + usize(1));
        } else {
            break grain;
        }
    };
    let echoed = settled;
    let displaced = if (usize(8) < echoed) {
        loan_write(capsule@ember) { |permit| replace(permit, u8(101)) }
    } else {
        loan_write(capsule@ember) { |permit| replace(permit, u8(19)) }
    };
    let after = capsule@ember;
    let still = capsule@reserve;
    displaced;
    unit
}
