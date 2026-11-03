fn r1120_requires_write(permission: ref<write, usize>) -> unit {
    unit
}

fn r1120_unauthorized_upgrade(permission: ref<read, usize>) -> unit {
    r1120_requires_write(permission)
}
