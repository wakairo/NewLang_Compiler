fn touch(x:ref<read,u8>)->unit{unit} fn main()->unit{let x=u8(7);loan_read(x){|r|touch(r);unit};unit}
