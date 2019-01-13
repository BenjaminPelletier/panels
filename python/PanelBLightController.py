import logging
import math
import random
import time
from blinker import Blinker
from dmx import DmxClient
from panels import PanelB
from scroller import Scroller


logger = logging.getLogger(__name__)
logger.setLevel(logging.INFO)
logger.addHandler(logging.StreamHandler())

PAN_CHANNEL = 1
PAN_MIN = 0
PAN_MAX = 540
PAN_FULL_SPAN = 540
PAN_SPEED_SWITCHES = ('LR', 'LY', 'LG')
PAN_SPEEDS = (10, 30, 70)

TILT_CHANNEL = 3
TILT_MIN = 0
TILT_MAX = 180
TILT_FULL_SPAN = 180
TILT_SPEED_SWITCHES = ('RR', 'RY', 'RG')
TILT_SPEEDS = (10, 30, 60)

JOY_UP = 'JU'
JOY_DOWN = 'JD'
JOY_LEFT = 'JL'
JOY_RIGHT = 'JR'

COLOR_CHANNEL = 5
COLOR_SWITCH = '20'
COLORS = tuple(range(0, 140, 10))

BRIGHTNESS_CHANNEL = 8
BRIGHTNESS_SWITCH = '21'

GOBO_CHANNEL = 6
GOBO_SWITCH = '22'
GOBOS = tuple(range(0, 64, 8))

RESET_SWITCH = '11'

AUTO_CHANNEL = 10
AUTO_SWITCHES = ('00', '01', '02')
AUTO_MODES = (60, 160, 135)
AUTO_DISABLE = 0


class SystemState(object):
    def __init__(self, panel, dmx):
        self.panel = panel
        self.dmx = dmx
        self.pan = Scroller(PAN_MIN, PAN_MAX)
        self.tilt = Scroller(TILT_MIN, TILT_MAX)
        self.color_index = 0
        self.gobo_index = 0
        self.brightness = Scroller(0, 255, 255 / 3.0)
        self.brightness_direction = 1
        self.is_auto = False

    def switch_changed(self, switch, value):
        if switch == JOY_UP:
            self.set_auto_mode(False)
            self.tilt.scroll_down(value)
        elif switch == JOY_DOWN:
            self.set_auto_mode(False)
            self.tilt.scroll_up(value)
        elif switch == JOY_RIGHT:
            self.set_auto_mode(False)
            self.pan.scroll_up(value)
        elif switch == JOY_LEFT:
            self.set_auto_mode(False)
            self.pan.scroll_down(value)
        elif switch in PAN_SPEED_SWITCHES and value:
            self.set_auto_mode(False)
            for sw, speed in zip(PAN_SPEED_SWITCHES, PAN_SPEEDS):
                self.panel.set_indicator(sw, sw == switch)
                if sw == switch:
                    self.pan.speed = speed
        elif switch in TILT_SPEED_SWITCHES and value:
            self.set_auto_mode(False)
            for sw, speed in zip(TILT_SPEED_SWITCHES, TILT_SPEEDS):
                self.panel.set_indicator(sw, sw == switch)
                if sw == switch:
                    self.tilt.speed = speed
        elif switch == COLOR_SWITCH:
            self.set_auto_mode(False)
            self.panel.set_indicator(switch, value)
            if value:
                self.color_index += 1
                if self.color_index >= len(COLORS):
                    self.color_index = 0
                self.dmx[COLOR_CHANNEL] = COLORS[self.color_index]
        elif switch == GOBO_SWITCH:
            self.set_auto_mode(False)
            self.panel.set_indicator(switch, value)
            if value:
                self.gobo_index += 1
                if self.gobo_index >= len(GOBOS):
                    self.gobo_index = 0
                self.dmx[GOBO_CHANNEL] = GOBOS[self.gobo_index]
        elif switch == BRIGHTNESS_SWITCH:
            self.set_auto_mode(False)
            self.panel.set_indicator(switch, value)
            self.brightness.scroll(self.brightness_direction, value)
            if value:
                self.brightness_direction = -self.brightness_direction
        elif switch == RESET_SWITCH:
            self.panel.set_indicator(switch, value)
            if value:
                self.set_auto_mode(False)
                self.brightness.value = 0
                self.brightness_direction = 1
                self.color_index = 0
                self.dmx[COLOR_CHANNEL] = COLORS[0]
                self.gobo_index = 0
                self.dmx[GOBO_CHANNEL] = GOBOS[0]
        elif switch in AUTO_SWITCHES and value:
            self.set_auto_mode(True)
            for sw, mode in zip(AUTO_SWITCHES, AUTO_MODES):
                self.panel.set_indicator(sw, sw == switch)
                if sw == switch:
                    self.dmx[AUTO_CHANNEL] = mode

    def set_auto_mode(self, is_auto):
        if self.is_auto and not is_auto:
            self.dmx[AUTO_CHANNEL] = AUTO_DISABLE
            for switch in AUTO_SWITCHES:
                self.panel.set_indicator(switch, False)
        elif not self.is_auto and is_auto:
            pass
        self.is_auto = is_auto

    def update(self):
        if not self.is_auto:
            self.dmx.set16(PAN_CHANNEL, self.pan.value / PAN_FULL_SPAN)
            self.dmx.set16(TILT_CHANNEL, self.tilt.value / TILT_FULL_SPAN)
            self.dmx[BRIGHTNESS_CHANNEL] = self.brightness.value


logger.info("Initializing panel connection")
panel = PanelB('/dev/ttyUSB0')
logger.info("Initializing DMX server connection")
dmx = DmxClient('http://192.168.1.79:8080')
dmx.start()

system = SystemState(panel, dmx)

while True:
    changed_switches = panel.changed_switches()
    for switch in changed_switches:
        system.switch_changed(switch, panel.get_switch(switch))
    system.update()
    time.sleep(0.1)


logger.info("Closing panel connection")
del panel

logger.info("Done.")
