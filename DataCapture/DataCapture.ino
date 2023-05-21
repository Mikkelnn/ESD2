#include <driver/spi_master.h>
#include <soc/soc.h>

#define SH_LD 22 // LOW triggers ADC sampeling - HIGH enabels serial shift (data is ready from ADC after <= 2.5us, set PWM to 30% LOW @ 80Khz)
#define DATA_IRQ 21 // Loopback of SH_LD for interrupt on RISING-edge, indicates data is ready in PISO registers to be serially clocked into the MCU
#define VSPI_CLK 18 // Clock to shift serial data out from PISO registers (should be running @ >= 10Mhz)
#define VSPI_MISO 19 // Data in from PISO register
#define TRANSDUCER_PWM 5 // PWM signal to transducer

#define SAMPLE_FREQUENCY 80000 // 80Khz
#define DATA_BUFFER (SAMPLE_FREQUENCY / 5) // samples for 200ms
#define ADC_BIT_RESOLUTION 8
#define N_CHANNELS 4

#define PULSE_WIDTH_MS 1 // 20ms pulse length ~6.86m sound travel

static spi_device_handle_t spi_device;

void fInitializeSPI_Channel(int spiCLK, int spiMISO)
{
  spi_bus_config_t bus_config = { };
  bus_config.sclk_io_num = spiCLK; // CLK
  bus_config.mosi_io_num = -1; // MOSI
  bus_config.miso_io_num = spiMISO; // MISO
  bus_config.quadwp_io_num = -1; // Not used
  bus_config.quadhd_io_num = -1; // Not used
  spi_bus_initialize(HSPI_HOST, &bus_config, false);
}

void fInitializeSPI_Devices(spi_device_handle_t &h, int csPin)
{
  spi_device_interface_config_t dev_config = { };  // initializes all field to 0
  dev_config.address_bits     = 0;
  dev_config.command_bits     = 0;
  dev_config.dummy_bits       = 0;
  dev_config.mode             = 1; // Data sampled on the falling edge and shifted out on the rising edge // 0; // Data sampled on rising edge and shifted out on the falling edge
  dev_config.duty_cycle_pos   = 0;
  dev_config.cs_ena_posttrans = 0;
  dev_config.cs_ena_pretrans  = 0;
  dev_config.clock_speed_hz   = SPI_MASTER_FREQ_20M; // 20Mhz
  dev_config.input_delay_ns   = 0;
  dev_config.spics_io_num     = csPin;
  dev_config.flags            = 0;
  dev_config.queue_size       = 1;
  dev_config.pre_cb           = NULL;
  dev_config.post_cb          = NULL;
  spi_bus_add_device(HSPI_HOST, &dev_config, &h);  
}

void fReadSPIdata(spi_device_handle_t &h, void * data, int length)
{
    spi_transaction_t trans_desc = { };
    trans_desc.addr =  0;
    trans_desc.cmd = 0;
    trans_desc.flags = 0;
    trans_desc.length = length; // total data bits
    trans_desc.tx_buffer = NULL;
    trans_desc.rxlength = length; // Number of bits NOT number of bytes
    trans_desc.rx_buffer = data;

    spi_device_polling_transmit(h, &trans_desc);
}

uint16_t data_pos = 0;
uint32_t buffer[DATA_BUFFER];

// setting PWM properties for ADC/PISO registers
const int ADC_PWM_freq = 80000;
const int ADC_PWM_Channel = 0;
const int ADC_PWM_resolution = 8;
const int Dutycycle_76_Percent = 2.55 * 76.0; // LOW ~3us @80khz

// setting PWM properties for transducer PWM
const int Transducer_freq = 39000;
const int Transducer_PWM_Channel = 2; // channel 0 and 1 is shared
const int Transducer_resolution = 8;
const int Dutycycle_50_Percent = 127;

TaskHandle_t xHandle_vTaskAcquireData, xHandle_vTaskPulseControl;

void vTaskAcquireData(void *pvParameters)
{
  // configure PWM for ADC and PISO registers
  ledcSetup(ADC_PWM_Channel, ADC_PWM_freq, ADC_PWM_resolution);  
  // attach the channel to the GPIO to be controlled
  ledcAttachPin(SH_LD, ADC_PWM_Channel);

  bool currentState, prevState = 0;
  for(;;)
  {
    // Stop after buffer is filled, wait untill resumed by control logic
    data_pos = 0; // reset buffer pointer
    ledcWrite(ADC_PWM_Channel, 0); // turn OFF the PWM signal
    vTaskSuspend(NULL); // Wait untill triggered from control logic    
    ledcWrite(ADC_PWM_Channel, Dutycycle_76_Percent); // set the duty cycle of the PWM channel

    while (data_pos < DATA_BUFFER) {
      // determine if the IRQ pin is HIGH or LOW
      currentState = (REG_READ(GPIO_IN_REG) & (1 << DATA_IRQ)) > 0;
      if (prevState == currentState) continue; // same -> skip
      else if (prevState == 0) {
        // RISING edge registered
        // if the IRQ pin has changed and previously was LOW -> RISING edge
        prevState = currentState;
        continue;
      }

      // FALLING edge registered
      // if the IRQ pin has changed and previously was HIGH -> FALLING edge
      prevState = currentState;

      // start SPI data setup and transfer
      fReadSPIdata(spi_device, &(buffer[data_pos++]), N_CHANNELS * ADC_BIT_RESOLUTION);
    }
  }
}


// Control the ultrasonic pulse duration, transmits a single pulse
void vTaskPulseControl(void *pvParameters) {
  // configure PWM for the transducer
  ledcSetup(Transducer_PWM_Channel, Transducer_freq, Transducer_resolution);
  // attach the channel to the GPIO to be controlled
  ledcAttachPin(TRANSDUCER_PWM, Transducer_PWM_Channel);

  for (;;) {
    // End ultrasonic pulse transmission
    ledcWrite(Transducer_PWM_Channel, 0); // Off

    // Wait untill triggered
    vTaskSuspend(NULL); 

    // Start ultrasonic pulse transmission
    ledcWrite(Transducer_PWM_Channel, Dutycycle_50_Percent);

    // Wait for the pulse duration/width
    delayMicroseconds(PULSE_WIDTH_MS * 1000);
  }
}

void setup() {
  // Set ÍRQ pin to input
  // Used to determine if the ADC_PWM has started new ADC conversion
  pinMode(DATA_IRQ, INPUT);

  // Configure SPI
  fInitializeSPI_Channel(VSPI_CLK, VSPI_MISO);
  fInitializeSPI_Devices(spi_device, -1);
  spi_device_acquire_bus(spi_device, portMAX_DELAY);

  // Register tasks with highest priority pinned specific core
  xTaskCreatePinnedToCore(vTaskAcquireData, "AcquireData", 1000, NULL, configMAX_PRIORITIES - 1, &xHandle_vTaskAcquireData, 0);
  xTaskCreatePinnedToCore(vTaskPulseControl, "PulseControl", 1000, NULL, configMAX_PRIORITIES - 1, &xHandle_vTaskPulseControl, 1);

  // Initialize serial connection
  Serial.begin(115200);
}

void loop() {
  if (!Serial.available()) return;

  char input = Serial.read();
  if (input == 'r') {
    // Start the ultrasonic pulse and data acquisition
    // Start ADC conversions and data aquisition
    vTaskResume(xHandle_vTaskAcquireData);
    // Start ultrasonic pulse
    vTaskResume(xHandle_vTaskPulseControl);
  } else if (input == 'd') {
    // transfer data as CSV - "ch0,ch1,ch2,ch3"
    for (int i = 0; i < DATA_BUFFER; i++) {
      for (int j = 0; j < N_CHANNELS; j++) {
        Serial.print((buffer[i] & (0xff << j*8)) >> j*8, DEC);
        if (j < N_CHANNELS-1) Serial.print(',');
        else Serial.println();
      }
    }
  } 
}

// https://www.espressif.com/sites/default/files/documentation/esp32_technical_reference_manual_en.pdf
// page 61 and 66
// https://www.esp32.com/viewtopic.php?t=1746