#include <driver/spi_master.h>
#include <soc/soc.h>

#define SH_LD 22 // LOW triggers ADC sampeling - HIGH enabels serial shift (data is ready from ADC after <= 2.5us, set PWM to 30% LOW @ 80Khz)
#define DATA_IRQ 21 // Loopback of SH_LD for interrupt on RISING-edge, indicates data is ready in PISO registers to be serially clocked into the MCU
#define VSPI_CLK 18 // Clock to shift serial data out from PISO registers (should be running @ >= 10Mhz)
#define VSPI_MISO 19 // Data in from PISO register

#define SAMPLE_FREQUENCY 80000 // 80Khz
#define DATA_BUFFER (SAMPLE_FREQUENCY / 5) // samples for 200ms
#define N_CHANNELS 4

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
  dev_config.input_delay_ns   = 0;//5;
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
uint64_t buffer[DATA_BUFFER];
uint64_t data = 0;

uint64_t count = 0;
uint64_t errors = 0;
uint8_t maxDiff = 0;
uint8_t tolerence = 10;

// setting PWM properties
const int freq = 80000;
const int PWM_Channel = 0;
const int resolution = 8;
const int Dutycycle_24_Percent = 2.55 * 76.0; // LOW ~3us @80khz


TaskHandle_t xHandle;
void vTaskGetData(void *pvParameters)
{
  bool current, prevState = 0;
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


    // data = 0;

    // REG_WRITE(GPIO_OUT_W1TS_REG, 1 << 5); // set GPIO 5 HIGH at IRQ start

    fReadSPIdata(spi_device, &data, 64);

    // count++;

    // uint8_t * channels = (uint8_t *)&data;
    // uint8_t min = 255, max = 0;
    // for (int i = 0; i < N_CHANNELS; i++){
    //   if (channels[i] < min) min = channels[i];
    //   if (channels[i] > max) max = channels[i];
    // }
    // uint8_t diff = max - min;
    // if (maxDiff < diff) maxDiff = diff;
    // if (diff > tolerence) errors++;    

    // REG_WRITE(GPIO_OUT_W1TC_REG, 1 << 5); // set GPIO 5 HIGH at IRQ start
  }
}

void IRAM_ATTR ISR_get_data() {
    data = 0;

    REG_WRITE(GPIO_OUT_W1TS_REG, 1 << 5); // set GPIO 5 HIGH at IRQ start

    fReadSPIdata(spi_device, &data/*buffer[data_pos++]*/, 64);

    count++;

    // uint8_t * channels = (uint8_t *)&data;
    // uint8_t min = 255, max = 0;
    // for (int i = 0; i < N_CHANNELS; i++){
    //   if (channels[i] < min) min = channels[i];
    //   if (channels[i] > max) max = channels[i];
    // }
    // uint8_t diff = max - min;
    // if (maxDiff < diff) maxDiff = diff;
    // if (diff > tolerence) errors++;    

    REG_WRITE(GPIO_OUT_W1TC_REG, 1 << 5); // set GPIO 5 HIGH at IRQ start
}

void setup() {
  pinMode(5, OUTPUT); // for DEBUG only!

  fInitializeSPI_Channel(VSPI_CLK, VSPI_MISO);
  fInitializeSPI_Devices(spi_device, -1);
  spi_device_acquire_bus(spi_device, portMAX_DELAY);

  // xTaskCreatePinnedToCore(vTaskGetData, "GetData", 1000, NULL, configMAX_PRIORITIES - 1, &xHandle, 0);

  Serial.begin(9600);
  Serial.println("init:");

  pinMode(DATA_IRQ, INPUT);
  // pinMode(SH_LD, OUTPUT);
  // digitalWrite(SH_LD, HIGH);

  // setup interrupt
  // attachInterrupt(DATA_IRQ, ISR_get_data, FALLING);


  // configure LED PWM functionalitites
  ledcSetup(PWM_Channel, freq, resolution);
  
  // attach the channel to the GPIO to be controlled
  ledcAttachPin(SH_LD, PWM_Channel);

  // set the duty cycle of the PWM channel
  ledcWrite(PWM_Channel, Dutycycle_24_Percent);
}

bool current, prevState = 0;
uint64_t lastTime = 0;
int interval_ms = 10000; // 10 seconds
void loop() {
  // if (millis() - lastTime > interval_ms) {
  //   lastTime = millis();

  //   Serial.print("Count: "); Serial.print(count, DEC); Serial.print(", Errors: "); Serial.print(errors, DEC); Serial.print(", maxDiff: "); Serial.print(maxDiff, DEC); Serial.println();
  // }

  // current = (REG_READ(GPIO_IN_REG) & (1 << DATA_IRQ)) > 0;
  //   if (prevState == current) return; // same - skip
  //   else if (prevState == 0) {
  //     // RISING edge registered
  //     prevState = current;
  //     return;
  //   }

  //   // FALLING edge registered 
  //   prevState = current;


  //   data = 0;

  //   // REG_WRITE(GPIO_OUT_W1TS_REG, 1 << 5); // set GPIO 5 HIGH at IRQ start

  //   fReadSPIdata(spi_device, &data/*buffer[data_pos++]*/, 64);

  //   count++;

  //   uint8_t * channels = (uint8_t *)&data;
  //   uint8_t min = 255, max = 0;
  //   for (int i = 0; i < N_CHANNELS; i++){
  //     if (channels[i] < min) min = channels[i];
  //     if (channels[i] > max) max = channels[i];
  //   }
  //   uint8_t diff = max - min;
  //   if (maxDiff < diff) maxDiff = diff;
  //   if (diff > tolerence) errors++;

    // REG_WRITE(GPIO_OUT_W1TC_REG, 1 << 5); // set GPIO 5 HIGH at IRQ start


  if (!Serial.available()) return;
  char input = Serial.read();
  if (input == 'r') {
    digitalWrite(SH_LD, LOW);
    delay(1);
    digitalWrite(SH_LD, HIGH);
    delay(1);
  }
  else if (input == 'l') digitalWrite(SH_LD, LOW);
  else if (input == 'h') digitalWrite(SH_LD, HIGH);
  if (input == 'q' || input == 'r') {
    data = 0;
    fReadSPIdata(spi_device, &data, 8*N_CHANNELS);
    
    for (int i = 0; i < N_CHANNELS; i++)
      Serial.println((data & (0xff << i*8)) >> i*8, DEC);

    Serial.println((data | ((uint64_t)1 << (8*N_CHANNELS + 1))), BIN);
  }
}

// https://www.espressif.com/sites/default/files/documentation/esp32_technical_reference_manual_en.pdf
// page 61 and 66
// https://www.esp32.com/viewtopic.php?t=1746