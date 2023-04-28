#include <driver/spi_master.h>
#include <soc/soc.h>

#define SH_LD 22 // LOW triggers ADC sampeling - HIGH enabels serial shift (data is ready from ADC after <= 2.5us, set PWM to 30% LOW @ 80Khz)
#define DATA_IRQ 21 // Loopback of SH_LD for interrupt on RISING-edge, indicates data is ready in PISO registers to be serially clocked into the MCU
#define VSPI_CLK 18 // Clock to shift serial data out from PISO registers (should be running @ >= 10Mhz)
#define VSPI_MISO 19 // Data in from PISO register
#define TRANSDUCER_PWM 5 // PWM signal to transducer

#define SAMPLE_FREQUENCY 80000 // 80Khz
#define DATA_BUFFER (SAMPLE_FREQUENCY / 5) // samples for 200ms
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

uint32_t data_pos = 0;
uint32_t buffer[DATA_BUFFER];
uint32_t data = 0;

// setting PWM properties for ADC/PISO registers
const int ADC_freq = 80000;
const int ADC_PWM_Channel = 0;
const int ADC_resolution = 8;
const int Dutycycle_24_Percent = 2.55 * 76.0; // LOW ~3us @80khz

// setting PWM properties for transducer PWM
const int Transducer_freq = 39000;
const int Transducer_PWM_Channel = 2;
const int Transducer_resolution = 8;
const int Dutycycle_50_Percent = 127;

TaskHandle_t xHandle_vTaskGetData, xHandle_vTaskPulseControl;

void vTaskGetData(void *pvParameters)
{
  bool current, prevState = 0;
  vTaskSuspend(NULL); // Wait untill resumed by control logic  
  ledcWrite(ADC_PWM_Channel, Dutycycle_24_Percent);
  for(;;)
  {
    current = (REG_READ(GPIO_IN_REG) & (1 << DATA_IRQ)) > 0;
    if (prevState == current) continue; // same - skip
    else if (prevState == 0) {
      // RISING edge registered
      prevState = current;
      continue;
    }

    // FALLING edge registered 
    prevState = current;

    // REG_WRITE(GPIO_OUT_W1TS_REG, 1 << 5); // set GPIO 5 HIGH at IRQ start

    fReadSPIdata(spi_device, &(buffer[data_pos++]), N_CHANNELS * 8);

    // Stop after buffer is filled, wait untill resumed by control logic
    if (data_pos >= DATA_BUFFER) {
      data_pos = 0;
      ledcWrite(ADC_PWM_Channel, 0);
      vTaskSuspend(NULL);
      ledcWrite(ADC_PWM_Channel, Dutycycle_24_Percent);
    }
  
    // REG_WRITE(GPIO_OUT_W1TC_REG, 1 << 5); // set GPIO 5 HIGH at IRQ start
  }
}


// Control the ultrasonic pulse duration and start of sampleing
// Transmits a single pulse and starts the sampeling for the specified time period
void vTaskPulseControl(void *pvParameters) {
    for (;;) {
      vTaskSuspend(NULL); // Wait untill triggered
      REG_WRITE(GPIO_OUT_W1TS_REG, 1 << 17); // set GPIO 5 HIGH at IRQ start

      vTaskResume(xHandle_vTaskGetData);
      ledcWrite(Transducer_PWM_Channel, Dutycycle_50_Percent); // Start transmission
      // vTaskDelay(PULSE_WIDTH_MS / portTICK_PERIOD_MS);
      delayMicroseconds(PULSE_WIDTH_MS * 1000);
      ledcWrite(Transducer_PWM_Channel, 0); // Off

      REG_WRITE(GPIO_OUT_W1TC_REG, 1 << 17); // set GPIO 5 HIGH at IRQ start
    }
}

void setup() {
  pinMode(17, OUTPUT); // for DEBUG only!
  pinMode(DATA_IRQ, INPUT); // Used to determine if the ADC_PWM

  fInitializeSPI_Channel(VSPI_CLK, VSPI_MISO);
  fInitializeSPI_Devices(spi_device, -1);
  spi_device_acquire_bus(spi_device, portMAX_DELAY);

  xTaskCreatePinnedToCore(vTaskGetData, "GetData", 1000, NULL, configMAX_PRIORITIES - 1, &xHandle_vTaskGetData, 0);
  xTaskCreatePinnedToCore(vTaskPulseControl, "PulseControl", 1000, NULL, configMAX_PRIORITIES - 1, &xHandle_vTaskPulseControl, 1);

  Serial.begin(115200);
  Serial.println("init:");
  

  // configure PWM for ADC and PISO registers
  ledcSetup(ADC_PWM_Channel, ADC_freq, ADC_resolution);  
  // attach the channel to the GPIO to be controlled
  ledcAttachPin(SH_LD, ADC_PWM_Channel);
  // set the duty cycle of the PWM channel
  ledcWrite(ADC_PWM_Channel, 0);

  // configure PWM for the transducer
  ledcSetup(Transducer_PWM_Channel, Transducer_freq, Transducer_resolution);
  // attach the channel to the GPIO to be controlled
  ledcAttachPin(TRANSDUCER_PWM, Transducer_PWM_Channel);
  // set the duty cycle of the PWM channel
  ledcWrite(Transducer_PWM_Channel, 0); // Off
}

// uint64_t lastTime = 0;
// int interval_ms = 10000; // 10 seconds
void loop() {
  // if (millis() - lastTime > interval_ms) {
  //   lastTime = millis();

  //   Serial.print("Count: "); Serial.print(count, DEC); Serial.print(", Errors: "); Serial.print(errors, DEC); Serial.print(", maxDiff: "); Serial.print(maxDiff, DEC); Serial.println();
  // }

  if (!Serial.available()) return;
  char input = Serial.read();
  if (input == 'r') vTaskResume(xHandle_vTaskPulseControl);
  else if (input == 'd') {
    for (int i = 0; i < 10 /*DATA_BUFFER*/; i++) {
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