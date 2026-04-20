
#![no_main]
#![no_std]
use rp235x_hal as hal;
#[hal::entry]
fn main() -> ! {
    let pac = hal::pac::Peripherals::take().unwrap();
    let dma = pac.DMA;
    dma.ch[0].ch_read_addr().write(|w| unsafe { w.bits(0xd0000004) });
    loop {}
}
