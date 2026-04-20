#![no_std]
#![no_main]

use defmt::*;
use defmt_rtt as _;
use embedded_hal::delay::DelayNs;
use embedded_hal::digital::OutputPin;
#[cfg(target_arch = "riscv32")]
use panic_halt as _;
#[cfg(target_arch = "arm")]
use panic_probe as _;

// Alias for our HAL crate
use hal::entry;
use hal::fugit::RateExtU32;
use embedded_hal::spi::SpiBus;
use hal::clocks::Clock;


#[cfg(rp2350)]
use rp235x_hal as hal;

#[cfg(rp2040)]
use rp2040_hal as hal;

// use bsp::entry;
// use bsp::hal;
// use rp_pico as bsp;

/// The linker will place this boot block at the start of our program image. We
/// need this to help the ROM bootloader get our code up and running.
/// Note: This boot block is not necessary when using a rp-hal based BSP
/// as the BSPs already perform this step.
#[unsafe(link_section = ".boot2")]
#[used]
#[cfg(rp2040)]
pub static BOOT2: [u8; 256] = rp2040_boot2::BOOT_LOADER_W25Q080;

/// Tell the Boot ROM about our application
#[unsafe(link_section = ".start_block")]
#[used]
#[cfg(rp2350)]
pub static IMAGE_DEF: hal::block::ImageDef = hal::block::ImageDef::secure_exe();

/// External high-speed crystal on the Raspberry Pi Pico 2 board is 12 MHz.
/// Adjust if your board has a different frequency
const XTAL_FREQ_HZ: u32 = 12_000_000u32;

/// Entry point to our bare-metal application.
///
/// The `#[hal::entry]` macro ensures the Cortex-M start-up code calls this function
/// as soon as all global variables and the spinlock are initialised.
///
/// The function configures the rp2040 and rp235x peripherals, then toggles a GPIO pin in
/// an infinite loop. If there is an LED connected to that pin, it will blink.
#[entry]
fn main() -> ! {
    info!("Program start");
    // Grab our singleton objects
    let mut pac = hal::pac::Peripherals::take().unwrap();

    // Set up the watchdog driver - needed by the clock setup code
    let mut watchdog = hal::Watchdog::new(pac.WATCHDOG);

    // Configure the clocks
    let clocks = hal::clocks::init_clocks_and_plls(
        XTAL_FREQ_HZ,
        pac.XOSC,
        pac.CLOCKS,
        pac.PLL_SYS,
        pac.PLL_USB,
        &mut pac.RESETS,
        &mut watchdog,
    )
    .unwrap();

    #[cfg(rp2040)]
    let mut timer = hal::Timer::new(pac.TIMER, &mut pac.RESETS, &clocks);

    #[cfg(rp2350)]
    let mut timer = hal::Timer::new_timer0(pac.TIMER0, &mut pac.RESETS, &clocks);

    // The single-cycle I/O block controls our GPIO pins
    let sio = hal::Sio::new(pac.SIO);

    // Set the pins to their default state
    let pins = hal::gpio::Pins::new(
        pac.IO_BANK0,
        pac.PADS_BANK0,
        sio.gpio_bank0,
        &mut pac.RESETS,
    );

    // SPI Slave Configuration
    // SPI1: RX=24, CS=25, SCK=26, TX=27
    let spi = hal::Spi::<_, _, _, 8>::new(pac.SPI1, (
        pins.gpio27.into_function::<hal::gpio::FunctionSpi>(), // TX
        pins.gpio24.into_function::<hal::gpio::FunctionSpi>(), // RX
        pins.gpio26.into_function::<hal::gpio::FunctionSpi>(), // SCK
    ));

    let mut spi = spi.init(
        &mut pac.RESETS,
        clocks.peripheral_clock.freq(),
        1_000_000_u32.Hz(), // 1 MHz - matches master
        &embedded_hal::spi::Mode {
            polarity: embedded_hal::spi::Polarity::IdleHigh,
            phase: embedded_hal::spi::Phase::CaptureOnSecondTransition,
        },
    );

    let _cs_pin = pins.gpio25.into_floating_input();

    // Initialize GPIO pins 10-17 as logic analyzer inputs
    let _g10 = pins.gpio10.into_floating_input();
    let _g11 = pins.gpio11.into_floating_input();
    let _g12 = pins.gpio12.into_floating_input();
    let _g13 = pins.gpio13.into_floating_input();
    let _g14 = pins.gpio14.into_floating_input();
    let _g15 = pins.gpio15.into_floating_input();
    let _g16 = pins.gpio16.into_floating_input();
    let _g17 = pins.gpio17.into_floating_input();

    let mut capture_buffer = [0u8; 1000];

    loop {
        let mut cmd_buf = [0u8; 1];
        let mut response_buf = [0u8; 1];

        // Read SPI command
        if let Ok(_) = spi.read(&mut cmd_buf) {
            let cmd = cmd_buf[0];
            
            // CMD = 0x01 (Start Capture)
            if cmd == 0x01 {
                // Actually read GPIO as fast as possible in a tight CPU loop
                // Pins 10-17 are correctly aligned if we shift by 10 maybe
                for _i in 0..100 {
                    // Loop unrolling for maximum CPU capture speed
                    capture_buffer[0] = (unsafe { (*hal::pac::SIO::PTR).gpio_in() }.read().bits() >> 10) as u8;
                    capture_buffer[1] = (unsafe { (*hal::pac::SIO::PTR).gpio_in() }.read().bits() >> 10) as u8;
                    capture_buffer[2] = (unsafe { (*hal::pac::SIO::PTR).gpio_in() }.read().bits() >> 10) as u8;
                    capture_buffer[3] = (unsafe { (*hal::pac::SIO::PTR).gpio_in() }.read().bits() >> 10) as u8;
                    capture_buffer[4] = (unsafe { (*hal::pac::SIO::PTR).gpio_in() }.read().bits() >> 10) as u8;
                    capture_buffer[5] = (unsafe { (*hal::pac::SIO::PTR).gpio_in() }.read().bits() >> 10) as u8;
                    capture_buffer[6] = (unsafe { (*hal::pac::SIO::PTR).gpio_in() }.read().bits() >> 10) as u8;
                    capture_buffer[7] = (unsafe { (*hal::pac::SIO::PTR).gpio_in() }.read().bits() >> 10) as u8;
                    capture_buffer[8] = (unsafe { (*hal::pac::SIO::PTR).gpio_in() }.read().bits() >> 10) as u8;
                    capture_buffer[9] = (unsafe { (*hal::pac::SIO::PTR).gpio_in() }.read().bits() >> 10) as u8;
                }
                
                // return length or something
                response_buf[0] = (capture_buffer.len() & 0xFF) as u8;
                let _ = spi.write(&response_buf);
            }
        }
    }
}

/// Program metadata for `picotool info`
#[unsafe(link_section = ".bi_entries")]
#[used]
pub static PICOTOOL_ENTRIES: [hal::binary_info::EntryAddr; 5] = [
    hal::binary_info::rp_cargo_bin_name!(),
    hal::binary_info::rp_cargo_version!(),
    hal::binary_info::rp_program_description!(c"Blinky Example"),
    hal::binary_info::rp_cargo_homepage_url!(),
    hal::binary_info::rp_program_build_attribute!(),
];

// End of file
