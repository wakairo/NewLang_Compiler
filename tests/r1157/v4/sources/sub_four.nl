struct OrbitTile { ember: u8, reserve: u8 }
fn main() -> unit {
    let capsule = OrbitTile { reserve: u8(66), ember: u8(37) };
    let before = capsule@reserve;
    let settled = loop (grain = usize(4)) {
        if (grain < usize(1)) {
            break grain;
        } else {
            continue(grain - usize(1));
        }
    };
    let echoed = settled;
    let displaced = if (echoed < usize(0)) {
        loan_write(capsule@reserve) { |permit| replace(permit, u8(59)) }
    } else {
        loan_write(capsule@reserve) { |permit| replace(permit, u8(104)) }
    };
    let after = capsule@reserve;
    let still = capsule@ember;
    displaced;
    unit
}
