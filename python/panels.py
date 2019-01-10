import logging
import serial
import threading
import time


logger = logging.getLogger(__name__)
logger.setLevel(logging.DEBUG)
logger.addHandler(logging.StreamHandler())

class Colors:
    OFF = 0
    RED = 1
    GREEN = 2
    BLUE = 4


class Panel(object):
    SWITCHES = {}
    BINARY_INDICATORS = {}
    COLORED_LEDS = {}
    ANALOGS = {}
    SERVOS = {}
    ANALOG_DIGITS = '0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ'
    SERVO_DIGITS = 'ABCDEFGHIJKLMNOPQRSTUVWXYZ'
    SERVO_BASE = 14
    SWITCH_PREFIX = 'Switch '
    ANALOG_PREFIX = 'Analog '
    READY_KEY = 'Ready'

    def __init__(self, port):
        self._lock = threading.Lock()

        self._ser = serial.Serial(port, baudrate=115200)

        self._values = {}
        self._value_events = {}
        self._changed_keys = set()

        t = threading.Thread(target=self._listen_loop)
        t.daemon = True
        t.start()

        logger.info("Waiting for READY")
        self.wait_ready()

        logger.info("Initializing switches")
        for switch in self.SWITCHES:
            print(switch)
            self.read_switch(switch)
        logger.info("Initializing analogs")
        for analog in self.ANALOGS:
            self.read_analog(analog)
        logger.info("Initialization complete")

    def send_command(self, cmd):
        logger.debug("Command: " + cmd)
        self._ser.write((cmd + '\n').encode('ascii'))

    def wait_ready(self):
        self._get_value(self.READY_KEY, '*READY')

    def set_indicator(self, indicator, value):
        if indicator in self.BINARY_INDICATORS:
            cmd = 'S%s%s' % (indicator, '1' if value else '0')
        elif indicator in self.COLORED_LEDS:
            cmd = 'S%s%s' % (indicator, value)
        else:
            raise ValueError("Indicator '%s' does not exist in %s" % (indicator, type(self).__name__))
        self.send_command(cmd)

    def set_servo(self, servo, value):
        if servo not in self.SERVOS:
            raise ValueError("Servo '%s' does not exist in %s" % (servo, type(self).__name__))
        if value < 0 or value > 180:
            raise ValueError("Servo value %g is outside the range [0, 180]" % value)
        value = round(value)
        s = self.SERVO_DIGITS[value // self.SERVO_BASE] + self.SERVO_DIGITS[value % self.SERVO_BASE]
        self.send_command('T%s%s' % (servo, s))

    def read_switch(self, switch):
        if switch not in self.SWITCHES:
            raise ValueError("Switch '%s' does not exist in %s" % (switch, type(self).__name__))
        return self._get_value(self._switch_key(switch), 'QS%s' % switch)

    def get_switch(self, switch):
        if switch not in self.SWITCHES:
            raise ValueError("Switch '%s' does not exist in %s" % (switch, type(self).__name__))
        with self._lock:
            return self._values[self._switch_key(switch)]

    def read_analog(self, analog):
        if analog not in self.ANALOGS:
            raise ValueError("Analog input '%s' does not exist in %s" % (analog, type(self).__name__))
        return self._get_value(self._analog_key(analog), 'QA%s' % analog)

    def get_analog(self, analog):
        if analog not in self.ANALOGS:
            raise ValueError("Analog input '%s' does not exist in %s" % (analog, type(self).__name__))
        with self._lock:
            return self._values[self._analog_key(analog)]

    def changed_switches(self):
        return self._changed_values(self.SWITCH_PREFIX)

    def changed_analogs(self):
        return self._changed_values(self.ANALOG_PREFIX)

    def __del__(self):
        logger.info("Closing serial connection")
        self._ser.close()

    def _listen_loop(self):
        buf = b''
        while True:
            try:
                c = self._ser.read()
            except Exception as e:
                logger.error("Error in _listen_loop: " + str(e))
                break
            if c == b'\n':
                self._on_message(buf.decode('ascii'))
                buf = b''
            else:
                buf += c
        logger.info("Exited listen loop")

    def _on_message(self, msg):
        logger.debug("Incoming: " + msg)
        if msg[0] == 'S':
            switch = msg[1:3]
            if switch not in self.SWITCHES:
                logger.error("Received event '%s' for unknown switch %s in %s" % (msg, switch, type(self).__name__))
                return
            value = msg[3] == 'D'
            self._update_value(self._switch_key(switch), value)
        elif msg[0] == 'A':
            analog = msg[1]
            if analog not in self.ANALOGS:
                logger.error("Received event '%s' for unknown analog input %s in %s" % (msg, analog, type(self).__name__))
                return
            value = self.ANALOG_DIGITS.find(msg[2]) * len(self.ANALOG_DIGITS) + self.ANALOG_DIGITS.find(msg[3])
            self._update_value(self._analog_key(analog), value)
        elif msg == '*READY':
            self._update_value(self.READY_KEY, None)
        elif msg.startswith('*'):
            logger.warning(msg)
        else:
            logger.error("Unexpected incoming message: " + msg)

    def _switch_key(self, switch):
        return self.SWITCH_PREFIX + switch

    def _analog_key(self, analog):
        return self.ANALOG_PREFIX + analog

    def _update_value(self, key, value):
        with self._lock:
            events = self._value_events.pop(key, [])
            self._changed_keys.add(key)
        self._values[key] = value
        for event in events:
            event.set()

    def _get_value(self, key, cmd):
        response_received = threading.Event()
        with self._lock:
            events = self._value_events.get(key, [])
            events.append(response_received)
            self._value_events[key] = events
        self.send_command(cmd)
        response_received.wait()
        return self._values[key]

    def _changed_values(self, prefix):
        changed = []
        with self._lock:
            for key in list(self._changed_keys):
                if key.startswith(prefix):
                    changed.append(key[len(prefix):])
                    self._changed_keys.remove(key)
        return changed


class PanelA(Panel):
    SWITCHES = {'B0', 'B1', 'B2', 'G0', 'G1', 'G2', 'R0', 'R1', 'R2', 'Q0', 'Q1', 'Q2', 'Q3', 'A0', 'S0', 'S1'}
    BINARY_INDICATORS = {'B0', 'B1', 'B2', 'G0', 'G1', 'G2', 'R0', 'R1', 'R2', 'A0', 'S0', 'S1'}
    COLORED_LEDS = {'Q0', 'Q1', 'Q2', 'Q3'}
    ANALOGS = {'0'}
    SERVOS = {'0'}


class PanelB(Panel):
    SWITCHES = {'LR', 'LY', 'LG', 'RR', 'RY', 'RG', 'JU', 'JR', 'JD', 'JL', '00', '01', '02', '10', '11', '12', '20', '21', '22'}
    BINARY_INDICATORS = {'LR', 'LY', 'LG', 'RR', 'RY', 'RG', 'JC', '00', '01', '02', '10', '11', '12', '20', '21', '22'}
