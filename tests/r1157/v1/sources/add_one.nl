struct OrbitTile { ember: u8, reserve: u8 }
fn specimen() -> u8 {
    let capsule = OrbitTile { reserve: u8(29), ember: u8(74) };
    let before = capsule@reserve;
    let settled = loop (grain = usize(12)) {
        if (grain < usize(13)) {
            continue(grain + usize(1));
        } else {
            break grain;
        }
    };
    let echoed = settled;
    let displaced = if (usize(13) == echoed) {
        loan_write(capsule@reserve) { |permit| replace(permit, u8(92)) }
    } else {
        loan_write(capsule@reserve) { |permit| replace(permit, u8(11)) }
    };
    let after = capsule@reserve;
    let still = capsule@ember;
    displaced
}
