struct Link { next:Option<ptr<Link>>, payload:u8 }
struct Sample { lower:u8, upper:u8 }
fn take_tail(edge:ref<write,Option<ptr<Link>>>, address:ptr<Link>, allocation:Allocation, domain:LifetimeDomain)->LiveTail {
 let detached=replace(edge,Option<ptr<Link>>::None);
 return LiveTail{owned_ptr:address,owned_allocation:allocation,owned_domain:domain};
}
fn end_tail(address:ptr<Link>, allocation:Allocation, domain:LifetimeDomain)->unit {
 loan_read(domain){|stable|let live=ref_from_ptr(read,address,stable);unit};
 let vacant=loan_exclusive_read(domain){|ending|destroy(address,ending)};
 let storage=erase_slot<Link>(vacant);
 finalize_domain(domain);
 deallocate(allocation,storage);
 unit
}
fn phase_code(value:usize)->u8 {
 if(value < usize(2)){u8(67)}else{u8(191)}
}
fn main()->unit {
 
 match try_allocate_one<Link>(){
  None=>{unit},
  Some(first_backing)=>{
   let OneBacking{allocation,raw}=first_backing;
   let first_allocation=allocation;
   let first_slot=into_slot<Link>(raw);
   let first_domain=lifetime_domain();
   let first=loan_read(first_domain){|stable|
    initialize(first_slot,Link{next:Option<ptr<Link>>::None,payload:u8(14)},stable)
   };
   match try_allocate_one<Link>(){
    None=>{
     let vacant=loan_exclusive_read(first_domain){|ending|destroy(first,ending)};
     let storage=erase_slot<Link>(vacant);
     finalize_domain(first_domain);
     deallocate(first_allocation,storage);
     unit
    },
    Some(second_backing)=>{
     let OneBacking{allocation,raw}=second_backing;
     let second_allocation=allocation;
     let second_slot=into_slot<Link>(raw);
     let second_domain=lifetime_domain();
     let second=loan_read(second_domain){|stable|
      initialize(second_slot,Link{next:Option<ptr<Link>>::None,payload:u8(235)},stable)
     };
     let absent=loan_read(first_domain){|stable|
      let link=ref_from_ptr(write,first,stable);
      replace(link@next,Option<ptr<Link>>::Some(second))
     };
     let seen=loan_read(first_domain){|stable|
      let view=ref_from_ptr(read,first,stable);
      read(view@next)
     };
     match seen{
      None=>{unit},
      Some(address)=>{loan_read(second_domain){|stable|let view=ref_from_ptr(read,address,stable);unit}},
     };
     let LiveTail{owned_ptr,owned_allocation,owned_domain}=loan_read(first_domain){|stable|
      let link=ref_from_ptr(write,first,stable);
      take_tail(link@next,second,second_allocation,second_domain)
     };
     loan_read(owned_domain){|stable|let view=ref_from_ptr(read,owned_ptr,stable);unit};
     end_tail(owned_ptr,owned_allocation,first_domain);
     let vacant=loan_exclusive_read(first_domain){|ending|destroy(first,ending)};
     let storage=erase_slot<Link>(vacant);
     finalize_domain(first_domain);
     deallocate(first_allocation,storage);
     unit
    },
   }
  },
 };
 let original=Sample{lower:u8(43),upper:u8(229)};
 let initial=usize(0);
 let start=initial;
 let finish=loop(phase=start){
  if(phase < usize(4)){
   let argument=phase;
   let incoming=phase_code(argument);
   let old=loan_write(original@lower){|writer|replace(writer,incoming)};
   let old_copy=old;
   old_copy;
   let current=original@lower;
   let sibling=original@upper;
   current;
   sibling;
   continue(phase + usize(1));
  }else{break phase;}
 };
 let final=original@lower;
 let sibling=original@upper;
 let iterations=finish;
 unit
}
