/*
  Keyboard logout

  This sketch demonstrates the Keyboard library.

  When you connect pin 2 to ground, it performs a logout.
  It uses keyboard combinations to do this, as follows:

  On Windows, CTRL-ALT-DEL followed by ALT-l
  On Ubuntu, CTRL-ALT-DEL, and ENTER
  On OSX, CMD-SHIFT-q

  To wake: Spacebar.

  Circuit:
  - Arduino Leonardo or Micro
  - wire to connect D2 to ground

  created 6 Mar 2012
  modified 27 Mar 2012
  by Tom Igoe

  This example is in the public domain.

  https://www.arduino.cc/en/Tutorial/BuiltInExamples/KeyboardLogout
*/QQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQ

void loop() {
  Keyboard.press('Q');
}
