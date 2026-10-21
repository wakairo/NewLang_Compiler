struct Meridian { east:u8, west:u8 }
fn transplant(slot:ref<write,u8>, incoming:u8)->u8 {
    replace(slot,incoming)
}
fn main()->unit {
    let primary=Meridian{west:u8(31),east:u8(7)};
    let secondary=Meridian{east:u8(40),west:u8(8)};
    let original_west=loan_write(primary@west){|handle|
        transplant(handle,u8(93))
    };
    let displaced=loan_write(secondary@east){|handle|
        replace(handle,original_west)
    };
    let primary_east=primary@east;
    let primary_west=primary@west;
    let secondary_east=secondary@east;
    let secondary_west=secondary@west;
    original_west;displaced;primary_east;primary_west;secondary_east;secondary_west;unit
}
