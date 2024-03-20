#pragma once

#include <Arduino.h>
#include <arduinoFFT.h>

#include <memory>

#include <pico/stdlib.h>
#include <hardware/adc.h>
#include <hardware/dma.h>

#include "Constants.h"

class SpectrumAnalyzer
{
  public:
    /**
     * @brief Construct a new Spectrum Analyzer object
     * 
     * @param irq Call dmaHandler in the function
     */
    SpectrumAnalyzer(irq_handler_t irq);
    ~SpectrumAnalyzer();
    void init(void);
    void startSampling(void);
    void dmaHandler(void);
    /**
     * @brief USE A MUTEX AROUND THIS
     * 
     */
    void update(void);
    uint16_t sampleCount(void) const;
    /**
     * @brief USE A MUTEX AROUND THIS
     * 
     * @return float* A pointer to the resulting FFT bins
     */
    float* bins() const;

  private:
    dma_channel_config cfg;
    uint dmaChannel;
    float *vReal;
    float *vImag;
    uint8_t *adcBuf;
    irq_handler_t irqHandler;
    bool hasData = false;
    std::unique_ptr<ArduinoFFT<float>> fft;
};

static mutex_t spectrumMtx;
static void dmaIrqHandler(void);

static SpectrumAnalyzer spectrum{dmaIrqHandler};

void dmaIrqHandler()
{
  spectrum.dmaHandler();
}