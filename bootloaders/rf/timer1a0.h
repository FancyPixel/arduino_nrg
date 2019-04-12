#ifndef _TIMER1A0_H
#define _TIMER1A0_H


class TIMER1A0
{ 
  public:
  
  /**
   * start
   *
   * Start T1A3 timer
   *
   * @param millis Milliseconds to be timed. Up to 2000 ms
   */
  ALWAYS_INLINE
  void start(unsigned int millis)
  {
    TA1CCR0 = 32.767 * millis;                // Max count
    TA1CTL = TASSEL_1 + MC_1 + TACLR;         // ACLK, upmode, clear TAR
  }
  
  /**
   * stop
   *
   * Stop T1A3 timer
   */
  ALWAYS_INLINE
  void stop(void)
  {
    TA1CTL = MC_0 + TACLR;                    // Halt timer
  }
  
  /**
   * timeout
   * 
   * Return end of timing
   * 
   * @return true if the timer ended.
   */
  ALWAYS_INLINE
  bool timeout(void)
  {
    return (TA1R == TA1CCR0);
  }
};

#endif
