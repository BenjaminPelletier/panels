import logging
import os
import threading
import time
import requests


logger = logging.getLogger(__name__)
logger.setLevel(logging.INFO)
logger.addHandler(logging.StreamHandler())


class DmxClient(object):
    def __init__(self, base_url):
        self.base_url = base_url
        self._dmx = bytearray(''.join(['00'] * 512), 'ascii')
        self._dirty = True
        self._lock = threading.RLock()
        self._active = False
        self._complete = threading.Event()

    def start(self):
        if self._active:
            return
        with self._lock:
            self._active = True
            self._complete = threading.Event()
        t = threading.Thread(target=self._update_loop)
        t.daemon = True
        t.start()

    def stop(self):
        with self._lock:
            e = self._complete
            self._active = False
        e.wait()

    def _update_loop(self):
        while self._active:
            with self._lock:
                if self._dirty:
                    try:
                        self._update_dmx()
                    except requests.HTTPError as e:
                        logger.warning('Error sending DMX update: ' + str(e))
                    self._dirty = False
            time.sleep(0.02)
        self._complete.set()

    def _update_dmx(self):
        url = os.path.join(self.base_url, 'dmx')
        with self._lock:
            r = requests.post(url, data=self._dmx)
        r.raise_for_status()

    def __setitem__(self, key, value):
        with self._lock:
            if isinstance(value, float):
                value = int(round(value))
            if value < 0:
                value = 0
            if value > 255:
                value = 255
            new_value = bytearray(format(value, '02x'), encoding='ascii')
            i0 = 2 * key
            for i in (0, 1):
                if self._dmx[i0 + i] != new_value[i]:
                    self._dmx[i0 + i] = new_value[i]
                    self._dirty = True

    def set16(self, channel, value):
        if isinstance(value, float):
            value = int(round(value * 0xffff))
        if value < 0:
            value = 0
        if value > 0xffff:
            value = 0xffff
        with self._lock:
            self[channel] = value // 256
            self[channel+1] = value % 256

    def __del__(self):
        self.stop()
