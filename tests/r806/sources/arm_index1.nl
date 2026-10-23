struct DispatchCell { next: Option<ptr<DispatchCell>>, payload: u8, }
struct PressurePair { idle: u8, peak: u8, }
fn update_byte(s:ref<write,u8>,v:u8)->u8{let prior=replace(s,v);return prior;}

fn unlink_tail(r:ref<write,Option<ptr<DispatchCell>>>,p:ptr<DispatchCell>,a:Allocation,d:LifetimeDomain)->LiveTail{
  let old=replace(r,Option<ptr<DispatchCell>>::None);
  return LiveTail{owned_ptr:p,owned_allocation:a,owned_domain:d};
}
fn dispose_tail(p:ptr<DispatchCell>,a:Allocation,d:LifetimeDomain)->unit{
  loan_read(d){|s|let r=ref_from_ptr(read,p,s);unit};
  let empty=loan_exclusive_read(d){|e|destroy(p,e)};
  let full=erase_slot<DispatchCell>(empty);
  finalize_domain(d);
  deallocate(a,full);
  unit
}
fn main()->unit{
  match try_allocate_one<DispatchCell>(){
    None=>{unit},
    Some(hpack)=>{
      let OneBacking{allocation,raw}=hpack;
      let ha=allocation;
      let hs=into_slot<DispatchCell>(raw);
      let hd=lifetime_domain();
      let hp=loan_read(hd){|s|initialize(hs,DispatchCell{next:Option<ptr<DispatchCell>>::None,payload:u8(23)},s)};
      match try_allocate_one<DispatchCell>(){
        None=>{
          let empty=loan_exclusive_read(hd){|e|destroy(hp,e)};
          let full=erase_slot<DispatchCell>(empty);
          finalize_domain(hd);
          deallocate(ha,full);
          unit
        },
        Some(tpack)=>{
          let OneBacking{allocation,raw}=tpack;
          let ta=allocation;
          let ts=into_slot<DispatchCell>(raw);
          let td=lifetime_domain();
          let tp=loan_read(td){|s|initialize(ts,DispatchCell{next:Option<ptr<DispatchCell>>::None,payload:u8(197)},s)};
          let old_none=loan_read(hd){|s|
            let w=ref_from_ptr(write,hp,s);
            replace(w@next,Option<ptr<DispatchCell>>::Some(tp))
          };
          let seen=loan_read(hd){|s|let r=ref_from_ptr(read,hp,s);read(r@next)};
          match seen{None=>{unit},Some(q)=>{loan_read(td){|s|let r=ref_from_ptr(read,q,s);unit}}};
          let packet=loan_read(hd){|s|
            let w=ref_from_ptr(write,hp,s);
            unlink_tail(w@next,tp,ta,td)
          };
          // Independent T operations are inside successful H arm AFTER the H owner call.
          let alpha=PressurePair{idle:u8(33),peak:u8(147)};
          let beta=PressurePair{idle:u8(219),peak:u8(52)};
          let old_copy=loan_write(alpha@peak){|w|update_byte(w,u8(61))};
          let evicted=loan_write(beta@idle){|w|replace(w,old_copy)};
          let updated_alpha=alpha@peak;
          let updated_beta=beta@idle;
          let sibling_alpha=alpha@idle;
          let sibling_beta=beta@peak;
          let LiveTail{owned_ptr,owned_allocation,owned_domain}=packet;
          loan_read(owned_domain){|s|let r=ref_from_ptr(read,owned_ptr,s);unit};
          dispose_tail(owned_ptr,owned_allocation,owned_domain);
          let empty=loan_exclusive_read(hd){|e|destroy(hp,e)};
          let full=erase_slot<DispatchCell>(empty);
          finalize_domain(hd);
          deallocate(ha,full);
          unit
        },
      }
    },
  }
}
