fn r1120_needs_read(permission: ref<read, usize>) -> unit {
    unit
}

fn r1120_cannot_implicitly_borrow(observed: usize) -> unit {
    r1120_needs_read(observed)
}
