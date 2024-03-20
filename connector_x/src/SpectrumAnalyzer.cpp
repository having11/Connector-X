#include "SpectrumAnalyzer.h"

SpectrumAnalyzer::SpectrumAnalyzer(irq_handler_t irq)
  : irqHandler{irq}
{
  vReal = new float[FFT::SampleCount];
  vImag = new float[FFT::SampleCount];
  adcBuf = new uint8_t[FFT::SampleCount];

  fft = std::make_unique<ArduinoFFT<float>>(vReal, vImag, FFT::SampleCount, FFT::SampleFrequencyHz);
}

SpectrumAnalyzer::~SpectrumAnalyzer()
{
  delete[] vReal;
  delete[] vImag;
  delete[] adcBuf;
}

void SpectrumAnalyzer::startSampling()
{
  adc_fifo_drain();
  adc_run(false);

  dma_channel_configure(dmaChannel, &cfg, adcBuf, &adc_hw->fifo, FFT::SampleCount, true);

  adc_run(true);
}

void SpectrumAnalyzer::update()
{
  if (hasData)
  {
    hasData = false;

    memset(vImag, 0, FFT::SampleCount);

    uint64_t sum = 0;
    for (uint16_t i = 0; i < FFT::SampleCount; i++)
      sum += adcBuf[i];

    float average = (float)sum / FFT::SampleCount;

    for (uint16_t i = 0; i < FFT::SampleCount; i++)
      vReal[i] = (float)adcBuf[i] - average;

    fft->windowing(FFTWindow::Hamming, FFTDirection::Forward);
    fft->compute(FFTDirection::Forward);

    startSampling();
  }
}

void SpectrumAnalyzer::dmaHandler()
{
  dma_channel_hw_t *hw = dma_channel_hw_addr(dmaChannel);
  dma_channel_acknowledge_irq0(dmaChannel);

  hasData = true;
}

uint16_t SpectrumAnalyzer::sampleCount() const
{
  return FFT::SampleCount;
}

float* SpectrumAnalyzer::bins() const
{
  return vReal;
}

void SpectrumAnalyzer::init()
{
  adc_gpio_init(FFT::AdcPin + FFT::CaptureChannel);
  adc_init();
  adc_select_input(FFT::CaptureChannel);
  adc_fifo_setup(
      true,    // Write each completed conversion to the sample FIFO
      true,    // Enable DMA data request (DREQ)
      1,       // DREQ (and IRQ) asserted when at least 1 sample present
      false,   // We won't see the ERR bit because of 8 bit reads; disable.
      true     // Shift each sample to 8 bits when pushing to FIFO
  );
  adc_set_clkdiv(FFT::ClockDivider);
  delay(1000);

  dmaChannel = dma_claim_unused_channel(true);
  cfg = dma_channel_get_default_config(dmaChannel);
  irq_set_exclusive_handler(DMA_IRQ_0, irqHandler);
  irq_set_enabled(DMA_IRQ_0, true);
  dma_channel_set_irq0_enabled(dmaChannel, true);

  channel_config_set_transfer_data_size(&cfg, DMA_SIZE_8);
  channel_config_set_read_increment(&cfg, false);
  channel_config_set_write_increment(&cfg, true);
  channel_config_set_dreq(&cfg, DREQ_ADC);
}