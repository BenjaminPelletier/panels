import logging
import math
import random
import time
from blinker import Blinker
from panels import PanelA


logger = logging.getLogger(__name__)
logger.setLevel(logging.INFO)
logger.addHandler(logging.StreamHandler())

RED_SWITCHES = ('R0', 'R1', 'R2')
GREEN_SWITCHES = ('G0', 'G1', 'G2')
BLUE_SWITCHES = ('B0', 'B1', 'B2')

THROTTLE = '0'
THROTTLE_ZERO = 20
RED_MIN = 70
RED_MAX = 120
RED_BOOST = 40
RED_RAMP = 0.8
RED_DWELL = 0.4
GREEN_MIN = 60
GREEN_MAX = 90
GREEN_RAMP = 2
GREEN_DWELL = 0.5


class SystemState(object):
    def __init__(self, panel):
        self.panel = panel
        self.red_blinker = Blinker((lambda: self.panel.set_indicator('S0', True),
                                    lambda: self.panel.set_indicator('S0', False)), 1)
        self.green_blinker = Blinker((lambda: self.panel.set_indicator('S1', True),
                                      lambda: self.panel.set_indicator('S1', False)), 1)
        self.red_ready = False
        self.green_ready = False
        self.panel.set_servo(THROTTLE, THROTTLE_ZERO)
        self.reset()

    def make_switch_target(self, switches, panel):
        current = tuple(panel.get_switch(s) for s in switches)
        targets = list(current)
        flip = list(range(len(switches)))
        random.shuffle(flip)
        nflip = random.randint(1, len(switches))
        for f in flip[0:nflip]:
            targets[f] = not targets[f]
        for switch, target in zip(switches, targets):
            panel.set_indicator(switch, panel.get_switch(switch) == target)
        return {switch: target for switch, target in zip(switches, targets)}

    def reset(self):
        self.red_target = self.make_switch_target(RED_SWITCHES, self.panel)
        self.green_target = self.make_switch_target(GREEN_SWITCHES, self.panel)

        self.blue_active = {s: not self.panel.get_switch(s) for s in BLUE_SWITCHES}
        if all(self.blue_active.values()):
            self.blue_active[BLUE_SWITCHES[random.randint(0, len(self.blue_active) - 1)]] = False
        for s in BLUE_SWITCHES:
            # self.panel.set_indicator(s, not self.blue_active[s])
            self.panel.set_indicator(s, False)

    def switches_changed(self, changed_switches):
        for switch in changed_switches:
            if switch in RED_SWITCHES:
                panel.set_indicator(switch, panel.get_switch(switch) == self.red_target[switch])
            if switch in GREEN_SWITCHES:
                panel.set_indicator(switch, panel.get_switch(switch) == self.green_target[switch])
            if switch in BLUE_SWITCHES:
                self.blue_active[switch] = True
                panel.set_indicator(switch, True)
        red_ready = all(panel.get_switch(k) == v for k, v in self.red_target.items())
        green_ready = all(panel.get_switch(k) == v for k, v in self.green_target.items())

        if red_ready:
            self.red_blinker.blink(4)
            if 'S0' in changed_switches and panel.get_switch('S0'):
                # Red launch button pressed
                peak_max = RED_MAX
                if self.boost():
                    peak_max += RED_BOOST
                peak = RED_MIN + self.read_slider() * (RED_MAX - RED_MIN)
                self.launch('S0', RED_RAMP, peak, RED_DWELL)
                self.reset()
        else:
            self.red_blinker.stop()

        if green_ready:
            self.green_blinker.blink(4)
            if 'S1' in changed_switches and panel.get_switch('S1'):
                # Green launch button pressed
                peak = GREEN_MIN + self.read_slider() * (GREEN_MAX - GREEN_MIN)
                self.launch('S1', GREEN_RAMP, peak, GREEN_DWELL)
                self.reset()
        else:
            self.green_blinker.stop()

    def read_slider(self):
        x = self.panel.read_analog('0')
        v = 0.000770032 * math.exp(0.00628978 * x) - 0.00127362
        return min(max(v, 0), 1)

    def boost(self):
        #return all(self.blue_active) and all(not self.panel.get_switch(s) for s in BLUE_SWITCHES)
        return all(not self.panel.get_switch(s) for s in BLUE_SWITCHES)

    def launch(self, launch_button, ramp, peak, hold):
        for s in self.panel.BINARY_INDICATORS:
            self.panel.set_indicator(s, s == launch_button)
        for q in self.panel.COLORED_LEDS:
            self.panel.set_indicator(q, '1')
        ramp_indicators = (('Q0', '3'), ('Q1', '3'), ('Q2', '3'), ('Q3', '3'), ('Q0', '2'), ('Q1', '2'), ('Q2', '2'), ('Q3', '2'), ('Q0', '7'), ('Q1', '7'), ('Q2', '7'))
        for i in range(11):
            self.panel.set_servo(THROTTLE, i * (peak - THROTTLE_ZERO) / 10 + THROTTLE_ZERO)
            time.sleep(ramp / 10)
            self.panel.set_indicator(ramp_indicators[i][0], ramp_indicators[i][1])
        self.panel.set_indicator('Q3', '7')
        if self.boost():
            self.panel.set_indicator('A0', '1')
        time.sleep(max(0, hold - 0.1))
        for q in self.panel.COLORED_LEDS:
            self.panel.set_indicator(q, '0')
        self.panel.set_servo(THROTTLE, THROTTLE_ZERO)

    def update(self):
        self.red_blinker.update()
        self.green_blinker.update()


logger.info("Initializing panel connection")
panel = PanelA('/dev/ttyACM0')
system = SystemState(panel)

while True:
    changed_switches = panel.changed_switches()
    if changed_switches:
        system.switches_changed(changed_switches)
    system.update()


logger.info("Closing panel connection")
del panel

logger.info("Done.")
