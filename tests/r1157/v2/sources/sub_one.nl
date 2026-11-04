struct OrbitTile { ember: u8, reserve: u8 }
fn main() -> u8 {
    let capsule = OrbitTile { reserve: u8(94), ember: u8(48) };
    let before = capsule@ember;
    let settled = loop (grain = usize(1)) {
        if (grain == usize(0)) {
            break grain;
        } else {
            continue(grain - usize(1));
        }
    };
    let echoed = settled;
    let displaced = if (usize(0) == echoed) {
        loan_write(capsule@ember) { |permit| replace(permit, u8(31)) }
    } else {
        loan_write(capsule@ember) { |permit| replace(permit, u8(72)) }
    };
    let after = capsule@ember;
    let still = capsule@reserve;
    displaced
}
